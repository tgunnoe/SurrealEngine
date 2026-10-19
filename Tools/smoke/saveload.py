#!/usr/bin/env python3
"""Run one game's save, quit and load chain across three maps and report what survives.

A chain is four engine processes on one user directory: play the first map and
set a known state, save and quit; start again, load that save, check, travel to
the second map, check, save and quit; load that save (made on the second map),
check, travel to the third map, check, save and quit; load the last save and
check once more. Each process's smoke log and engine log are parsed, and the
report says per hop which items the load restored, which the travel carried,
which were lost and which were gained.

Required environment (read from the caller, never set here):
  SURREALWIDGETS_DISPLAY_BACKEND=X11
  LD_LIBRARY_PATH must include the Vulkan loader, the SDL3 library and
  /run/opengl-driver/lib, as for sweep.py.

The driver never touches the caller's configuration: the engine runs with
XDG_CONFIG_HOME set to --config-root, where it keeps its settings and the
plaintext engine log. See README.md for the report schema.
"""
import argparse
import json
import os
import re
import shutil
import signal
import subprocess
import sys
from collections import Counter
from dataclasses import dataclass
from pathlib import Path

# Isolated mode (python3 -I) puts no directory on sys.path.
sys.path.insert(0, str(Path(__file__).resolve().parent))
from sweep import KILL_GRACE_SECONDS, MAP_SUFFIXES, PORTABLE_GAME, REPO_ROOT, classify, parse_log  # noqa: E402

STEP_NAMES = ("save1", "load1-travel", "load2-travel", "load3")
# The check windows each step owes, in log order. A load step's first window
# (before its load line) holds no check, so it is not owed.
STEP_WHERES = (("after-state",), ("after-load", "after-travel"),
               ("after-load", "after-travel"), ("after-load",))
CHECK_PATH = tuple((name, where) for name, wheres in zip(STEP_NAMES, STEP_WHERES, strict=True) for where in wheres)
TRAVELLING_STEPS = (1, 2)

DEFAULT_SECONDS = 60
# Seconds between a load or a travel and the commands that read its result.
SETTLE_SECONDS = 5
# The engine writes a save at the end of the frame the command ran in; the
# gap lets the last check's output settle before the file is written.
BEFORE_SAVE_SECONDS = 10
# Seconds between the commands of one check group.
CHECK_GAP_SECONDS = 1
ENGINE_LOG_NAME = "SE-Log-LastRun.txt"

TITLES = {"unreal": "Unreal Gold", "ut": "Unreal Tournament", "deusex": "Deus Ex"}


@dataclass(frozen=True)
class Chain:
    """One game's chain: three maps and the commands that set and read state."""
    maps: tuple[str, str, str]
    options: str  # URL options on every map, e.g. ?NumBots=0
    state: tuple[tuple[float, str], ...]  # (gap, command) after the first login
    load: str  # command template; {slot} is the slot number
    check: tuple[str, ...]  # the commands one check runs
    ready: tuple[str, ...]  # commands between a travel and its check
    trace: str | None  # --trace-script value for the travelling steps
    check_calls: frozenset[str]  # `call` functions a check records


CHAINS = {
    "unreal": Chain(
        maps=("Vortex2", "Nyleve", "Dig"),
        options="",
        state=((6, "summon UnrealShare.Stinger"), (1, "hold axis aForward 300 1.5")),
        load="call ClientTravel ?load={slot} 0 0",
        check=("ShowInventory",),
        ready=(),
        trace=None,
        check_calls=frozenset(),
    ),
    # A deathmatch waits for the player's ready signal, so Fire starts the match;
    # summon acts only with cheats on, which only IAmTheOne sets.
    "ut": Chain(
        maps=("DM-Deck16][", "DM-Turbine", "DM-Codex"),
        options="?NumBots=0",
        state=((5, "Fire"), (2, "IAmTheOne"), (1, "summon Botpack.ShockRifle"),
               (2, "hold axis aForward 300 1.5")),
        load="call ClientTravel ?load={slot} 0 0",
        check=("ShowInventory",),
        ready=("Fire",),
        trace="Inventory.TravelPreAccept",
        check_calls=frozenset(),
    ),
    # The harness reaches no Deus Ex flag value, so the goal (primary, which the
    # mission change keeps) and the note travel as the state witness.
    "deusex": Chain(
        maps=("01_NYC_UNATCOIsland", "01_NYC_UNATCOHQ", "02_NYC_BatteryPark"),
        options="",
        state=((5, "call AddGoal T3Goal 1"), (1, "NoteAdd T3Note"),
               (1, "call SetBoolFlagFromString T3Flag 1")),
        load="LoadGame {slot}",
        check=("ShowInventory", "call FindGoal T3Goal", "call GetNote None"),
        ready=(),
        trace=None,
        check_calls=frozenset({"FindGoal", "GetNote"}),
    ),
}


# --- the step schedule ---------------------------------------------------------------

def _group(first_gap, commands):
    return [(first_gap if i == 0 else CHECK_GAP_SECONDS, command) for i, command in enumerate(commands)]


def step_plan(chain, index):
    """The (gap, command) pairs of step `index`; a gap counts from the item before it.

    The first gap counts from the first login.
    """
    if index == 0:
        return list(chain.state) + _group(SETTLE_SECONDS, chain.check) + [(BEFORE_SAVE_SECONDS, "savegame 1")]
    plan = [(SETTLE_SECONDS, chain.load.format(slot=index))]
    plan += _group(SETTLE_SECONDS, chain.check)
    if index < len(STEP_NAMES) - 1:
        plan.append((SETTLE_SECONDS, f"call ClientTravel {chain.maps[index]}{chain.options} 2 1"))
        plan += [(SETTLE_SECONDS, command) for command in chain.ready]
        plan += _group(SETTLE_SECONDS, chain.check)
        plan.append((BEFORE_SAVE_SECONDS, f"savegame {index + 1}"))
    return plan


def _seconds_text(seconds):
    return str(int(seconds)) if seconds == int(seconds) else f"{seconds:.3f}".rstrip("0")


def build_exec(plan):
    """The --exec value for a plan: absolute times from the gaps, in written order."""
    at = 0.0
    items = []
    for gap, command in plan:
        if gap < 0:
            raise ValueError(f"negative gap {gap} before {command!r}")
        if ";" in command:
            raise ValueError(f"command {command!r} holds a semicolon, which --exec cannot carry")
        at += gap
        items.append(f"{_seconds_text(at)}:{command}")
    return ";".join(items)


def plan_end(plan):
    return sum(gap for gap, _ in plan)


# --- the engine log ------------------------------------------------------------------

_STAMP = re.compile(r"^\[[^\]]*\] ")
_LOADED = re.compile(r"<Surreal Engine>: Loaded game from slot (-?\d+): (.*)$")
_TRAVEL = re.compile(r"<Surreal Engine>: Client travel to (\S+)")
_SAVED = re.compile(r"<Surreal Engine>: Saved game to (.+)$")
_CALL = re.compile(r"<Surreal Engine>: call (\S+) returned (.*)$")
_INVENTORY = re.compile(r"ShowInventory:\s+Inv: (\S+)")
_WEAPON = re.compile(r"ShowInventory:\s+Weapon: (\S+)")
_OFFERED = re.compile(r"trace: (\S+) \S+ \S+: TravelPreAccept\(")
_SUMMON = re.compile(r"Summon: (?:Fabricate (\S+)|fabricated an? (\S+))")
_SKIPPING = "Skipping travel transfer"


def _window(where, map_name=None):
    return {"where": where, "map": map_name, "items": [], "calls": [], "weapon": None,
            "offered": [], "travel_skipped": False}


def _item(token):
    package, _, instance = token.rpartition(".")
    return {"package": package, "instance": instance, "class": re.sub(r"\d+$", "", instance) or instance}


def parse_engine_log(lines):
    """Split the engine log into check windows at its load, travel and save lines.

    Timestamps are level time, which restarts on a load or a travel, so only the
    position of a line orders it. The window before the first landmark is the
    check after the state commands; a landmark opens the window named for it.
    Only lines the engine itself wrote count as landmarks, never script output.
    """
    windows = [_window("after-state")]
    parsed = {"windows": windows, "loaded_slot": None, "loaded_map": None,
              "saved_to": None, "summoned": [], "skip_lines": []}
    for line in lines:
        body = _STAMP.sub("", line.rstrip())
        if m := _LOADED.match(body):
            parsed["loaded_slot"], parsed["loaded_map"] = int(m.group(1)), m.group(2)
            windows.append(_window("after-load", m.group(2)))
        elif m := _TRAVEL.match(body):
            windows.append(_window("after-travel", m.group(1).split("?")[0]))
        elif m := _SAVED.match(body):
            parsed["saved_to"] = m.group(1)
            windows.append(_window("after-save"))
        elif m := _CALL.match(body):
            windows[-1]["calls"].append({"function": m.group(1), "value": m.group(2).strip()})
        elif m := _INVENTORY.search(body):
            windows[-1]["items"].append(_item(m.group(1)))
        elif m := _WEAPON.search(body):
            windows[-1]["weapon"] = m.group(1)
        elif m := _OFFERED.search(body):
            offered = windows[-1]["offered"]
            if m.group(1).lower() not in (name.lower() for name in offered):
                offered.append(m.group(1))
        elif m := _SUMMON.search(body):
            parsed["summoned"].append(m.group(1) or m.group(2))
        elif _SKIPPING in body:
            windows[-1]["travel_skipped"] = True
            parsed["skip_lines"].append(body)
    return parsed


def parse_smoke_exec(lines):
    failed = []
    skipped = []
    for line in lines:
        kind, _, rest = line.partition(" ")
        if kind == "EXEC_FAILED":
            failed.append(rest)
        elif kind == "EXEC_SKIPPED":
            skipped.append(rest)
    return {"failed": failed, "skipped": skipped}


def assemble_checks(parsed, index, chain):
    """The checks step `index` owes, cut from a parsed engine log."""
    owed = STEP_WHERES[index]
    checks = []
    offered = []
    travel_skipped = False
    for window in parsed["windows"]:
        if window["where"] == "after-travel":
            offered += [name for name in window["offered"] if name not in offered]
            travel_skipped = travel_skipped or window["travel_skipped"]
        if window["where"] not in owed:
            continue
        calls = [call for call in window["calls"] if call["function"].lower() in
                 {function.lower() for function in chain.check_calls}]
        checks.append({"where": window["where"], "map": window["map"] or chain.maps[0],
                       "items": window["items"], "calls": calls, "weapon": window["weapon"]})
    return {"checks": checks, "offered": offered, "travel_skipped": travel_skipped}


# --- comparing and judging -----------------------------------------------------------

def compare(before, after):
    """Match two inventories by class name without case, counting repeats.

    A loaded item reads `Save<n>.X` or `<Map>.X` and a travelled one `<Map>.X`
    with another instance number, so the package and the number are ignored.
    """
    def matched(items, other):
        pool = Counter(i["class"].lower() for i in other)
        kept = []
        rest = []
        for i in items:
            key = i["class"].lower()
            if pool[key] > 0:
                pool[key] -= 1
                kept.append(i["class"])
            else:
                rest.append(i["class"])
        return kept, rest

    kept, lost = matched(before, after)
    _, gained = matched(after, before)
    return {"kept": kept, "lost": lost, "gained": gained}


def call_status(calls, function):
    """`found`, `lost` or the raw value of the last call of `function`; None if never called."""
    values = [c["value"] for c in calls if c["function"].lower() == function.lower()]
    if not values:
        return None
    value = values[-1]
    if value == "None":
        return "lost"
    if value in ("nothing", "<unprinted type>"):
        return value
    return "found"


def step_verdict(info, exit_code, timed_out, expected_map):
    """The sweep's verdict, with the expected map matched without case.

    The URL's spelling and the map package's can differ (`Nyleve`, `NyLeve`),
    and sweep.classify compares exactly.
    """
    spelled = next((m for m in info["maps_loaded"] if m.lower() == expected_map.lower()), expected_map)
    return classify(info, exit_code, timed_out, spelled)


def step_problem(step, index):
    """Why step `index` did not do what its schedule said, or None."""
    name = STEP_NAMES[index]
    # The engine writes its log only on exit, so a missing one means the process died first.
    dead = ", no engine log (the process died before writing one)" if step["engine_log_missing"] else ""
    if step["verdict"] != "pass":
        return f"{name}: verdict {step['verdict']}{dead}"
    if dead:
        return f"{name}: verdict pass{dead}"
    if step["exec_failed"]:
        return f"{name}: {len(step['exec_failed'])} scheduled command(s) failed: {'; '.join(step['exec_failed'])}"
    if step["exec_skipped"]:
        return f"{name}: {len(step['exec_skipped'])} scheduled command(s) skipped: {'; '.join(step['exec_skipped'])}"
    if index > 0 and step["loaded_slot"] != index:
        made = "none" if step["loaded_slot"] is None else f"slot {step['loaded_slot']}"
        return f"{name}: the load of slot {index} was not made (loaded: {made})"
    present = {c["where"] for c in step["checks"]}
    missing = [w for w in STEP_WHERES[index] if w not in present]
    if missing:
        return f"{name}: no check {', '.join(missing)}"
    if index < len(STEP_NAMES) - 1 and not step["saved_to"]:
        return f"{name}: the save of slot {index + 1} was not made"
    return None


def build_hops(steps):
    """One hop per consecutive pair of checks that were both made."""
    by_name = {s["step"]: s for s in steps}

    def find(step_name, where):
        step = by_name.get(step_name)
        return next((c for c in step["checks"] if c["where"] == where), None) if step else None

    hops = []
    for (from_step, from_where), (to_step, to_where) in zip(CHECK_PATH, CHECK_PATH[1:], strict=False):
        a = find(from_step, from_where)
        b = find(to_step, to_where)
        if a is None or b is None:
            continue
        by = "load" if to_where == "after-load" else "travel"
        hop = {"from": f"{from_step}/{from_where}", "to": f"{to_step}/{to_where}",
               "map_from": a["map"], "map_to": b["map"], "by": by, **compare(a["items"], b["items"])}
        for key, function in (("goal", "FindGoal"), ("note", "GetNote")):
            status = call_status(b["calls"], function)
            if status is not None:
                hop[key] = status
        if by == "travel":
            hop["offered"] = by_name[to_step]["offered"]
            hop["travel_skipped"] = by_name[to_step]["travel_skipped"]
        hops.append(hop)
    return hops


# --- the markdown --------------------------------------------------------------------

def _hop_label(hop):
    return f"{hop['from']} to {hop['to']}"


def _names(names):
    return ", ".join(names) if names else "-"


def verdict_sentence(report):
    game = report["chain"]
    title = TITLES.get(game, game)
    steps = report["steps"]
    hops = report["hops"]
    for index, step in enumerate(steps):
        problem = step_problem(step, index)
        if problem:
            return f"Incomplete: {problem}."
    if len(steps) < len(STEP_NAMES):
        return f"Incomplete: {len(steps)} of {len(STEP_NAMES)} steps ran."
    if len(hops) < len(CHECK_PATH) - 1:
        return f"Incomplete: {len(hops)} of {len(CHECK_PATH) - 1} hops were measured."

    findings = []
    for hop in hops:
        if hop["by"] == "load" and hop["map_from"].lower() != hop["map_to"].lower():
            findings.append(f"{_hop_label(hop)}: the save was made on {hop['map_from']}, "
                            f"the load arrived on {hop['map_to']}")
    if game == "ut":
        findings += [f"{_hop_label(h)} lost {_names(h['lost'])}" for h in hops if h["by"] == "load" and h["lost"]]
        if findings:
            return f"{title}: a load did not restore what was saved: {'; '.join(findings)}."
        dropped = sorted({c for h in hops if h["by"] == "travel" for c in h["lost"]})
        detail = f"lost on a travel: {', '.join(dropped)}" if dropped else "no listed class was lost on a travel"
        # The game rule can only be the cause when the engine did carry the items to the acceptance.
        silent = [h for h in hops if h["by"] == "travel" and (h["travel_skipped"] or not h["offered"])]
        if silent:
            where = "; ".join(_hop_label(h) for h in silent)
            return (f"{title}: loads restore what was saved; travels carry nothing: the engine skipped the "
                    f"transfer or offered no item to the acceptance on {where}, so the game rule that "
                    f"discards them in a deathmatch was not reached ({detail}).")
        return (f"{title}: loads restore what was saved; travels discard the carried inventory "
                f"by game rule ({detail}).")

    findings += [f"{_hop_label(h)} lost {_names(h['lost'])}" for h in hops if h["lost"]]
    findings += [f"{_hop_label(h)}: the travel transfer was skipped" for h in hops
                 if h["by"] == "travel" and h["travel_skipped"]]
    if game == "deusex":
        first = steps[0]["checks"][0]
        for key, function in (("goal", "FindGoal"), ("note", "GetNote")):
            status = call_status(first["calls"], function)
            if status != "found":
                findings.append(f"{steps[0]['step']}/{first['where']}: no {key} to carry "
                                f"({status or 'not read'})")
        for hop in hops:
            for key in ("goal", "note"):
                if hop.get(key) != "found":
                    findings.append(f"{_hop_label(hop)}: the {key} is {hop.get(key, 'not read')}")
    if findings:
        return f"{title}: not everything survives: {'; '.join(findings)}."
    maps = ", ".join(CHAINS[game].maps)
    what = "state, inventory and the Deus Ex goal and note" if game == "deusex" else "state and inventory"
    return f"{title}: {what} survive three maps ({maps})."


def _limits(game):
    limits = [
        "Items compare by class name without case; the package (`Save<n>` after a load on Unreal "
        "Gold and Unreal Tournament, the map's package after a travel and after a load on Deus Ex) "
        "and the instance number change and are ignored.",
        "Health and position are not read, beyond the `STATE` line of the smoke log.",
    ]
    if game == "ut":
        limits.append(
            "The deathmatch game type discards travelled inventory by game rule: "
            "`DeathMatchPlus.AcceptInventory` destroys every item but a ladder item and "
            "`AddDefaultInventory` gives the defaults again once the player is ready. A class listed "
            "as kept on a travel hop is therefore a default given again, not a carried item. The "
            "Offered column is the `--trace-script=Inventory.TravelPreAccept` trace, the witness "
            "that the engine carried the items to the acceptance (empty when the engine skipped the "
            "transfer); the destruction itself is not seen.")
    if game == "deusex":
        limits.append(
            "Inventory beyond the player's `NanoKeyRing` cannot be given from the harness (the game "
            "recreates the key ring on a travel), so the inventory witness is the key ring alone.")
        limits.append(
            "A flag's value is not read back: `SetBoolFlagFromString` is called, and survival is "
            "witnessed through a goal (`FindGoal`, added as a primary goal so the mission change "
            "keeps it) and a note (`GetNote`), which travel the same way.")
    return limits


def render_markdown(report):
    game = report["chain"]
    steps = report["steps"]
    hops = report["hops"]
    title = TITLES.get(game, game)
    maps = " to ".join(CHAINS[game].maps) if game in CHAINS else "?"
    out = [f"# {title}: save, quit and load across three maps", "",
           f"Chain: {maps}. Four engine processes on one user directory, {report['seconds']} seconds each, "
           "slots 1 to 3.", "",
           "## Steps", "",
           "| Step | Exit | Verdict | Maps loaded | Loaded slot | Saved to |",
           "| :--- | ---: | :--- | :--- | ---: | :--- |"]
    for s in steps:
        out.append(f"| {s['step']} | {s['exit_code']} | {s['verdict']} | {_names(s['maps_loaded'])} | "
                   f"{'-' if s['loaded_slot'] is None else s['loaded_slot']} | {s['saved_to'] or '-'} |")
    out += ["", "## Checks", "", "| Check | Map | Inventory |", "| :--- | :--- | :--- |"]
    for s in steps:
        for c in s["checks"]:
            listed = ", ".join(i["class"] for i in c["items"]) or "(nothing listed)"
            out.append(f"| {s['step']}/{c['where']} | {c['map']} | {listed} |")

    columns = ["Hop", "By", "Kept", "Lost", "Gained"]
    if game == "ut":
        columns.append("Offered")
    if game == "deusex":
        columns += ["Goal", "Note"]
    columns.append("Travel skipped")
    out += ["", "## Hops", "",
            "Kept reads as restored for a load and carried for a travel.", "",
            "| " + " | ".join(columns) + " |", "| :--- " * len(columns) + "|"]
    for hop in hops:
        cells = [_hop_label(hop), hop["by"], _names(hop["kept"]), _names(hop["lost"]), _names(hop["gained"])]
        if game == "ut":
            cells.append(_names(hop["offered"]) if hop["by"] == "travel" else "-")
        if game == "deusex":
            cells += [hop.get("goal", "-"), hop.get("note", "-")]
        cells.append(("yes" if hop["travel_skipped"] else "no") if hop["by"] == "travel" else "-")
        out.append("| " + " | ".join(cells) + " |")

    out += ["", "## Verdict", "", verdict_sentence(report), "", "## Limits of the claim", ""]
    out += [f"- {limit}" for limit in _limits(game)]
    return "\n".join(out) + "\n"


# --- running the engine --------------------------------------------------------------

def _step_record(name, chain, index, exec_text, trace, exit_code, timed_out, paths, portable):
    smoke_log, stdout, engine_log, live_engine_log = paths
    info, _ = parse_log(smoke_log)
    smoke_lines = smoke_log.read_text(encoding="utf-8", errors="replace").splitlines() if smoke_log.exists() else []
    expected = chain.maps[min(index, len(chain.maps) - 1)]
    engine_log_missing = not live_engine_log.exists()
    if engine_log_missing:
        engine_lines = []
    else:
        shutil.copyfile(live_engine_log, engine_log)
        engine_lines = engine_log.read_text(encoding="utf-8", errors="replace").splitlines()
    parsed = parse_engine_log(engine_lines)
    owed = assemble_checks(parsed, index, chain)
    execs = parse_smoke_exec(smoke_lines)

    def shown(path):
        return path.name if portable else str(path)

    step = {
        "step": name,
        "exit_code": exit_code,
        "verdict": step_verdict(info, exit_code, timed_out, expected),
        "maps_loaded": info["maps_loaded"],
        "loaded_slot": parsed["loaded_slot"],
        "loaded_map": parsed["loaded_map"],
        "saved_to": parsed["saved_to"],
        "checks": owed["checks"],
        "offered": owed["offered"],
        "travel_skipped": owed["travel_skipped"],
        "skip_lines": parsed["skip_lines"],
        "summoned": parsed["summoned"],
        "exec_failed": execs["failed"],
        "exec_skipped": execs["skipped"],
        "engine_log_missing": engine_log_missing,
        "exec": exec_text,
        "trace": trace,
        "log": shown(smoke_log),
        "stdout": shown(stdout),
        "engine_log": None if engine_log_missing else shown(engine_log),
    }
    step["problem"] = step_problem(step, index)
    return step


def run_step(exe, game, chain, index, seconds, user_dir, config_root, logdir, portable):
    name = STEP_NAMES[index]
    smoke_log = logdir / f"{name}.log"
    stdout = logdir / f"{name}.stdout"
    engine_log = logdir / f"{name}.engine.txt"
    live_engine_log = config_root / "SurrealEngine" / ENGINE_LOG_NAME
    # The engine writes its log only when it exits, so a killed child would leave
    # the previous step's file in place; a stale file must never be read as this step's.
    for stale in (smoke_log, engine_log, live_engine_log):
        stale.unlink(missing_ok=True)
    exec_text = build_exec(step_plan(chain, index))
    cmd = [str(exe), "--autostart", f"--exit-after={seconds}", f"--smoke-log={smoke_log}",
           f"--user-dir={user_dir}", f"--url={chain.maps[0]}{chain.options}", f"--exec={exec_text}"]
    trace = chain.trace if index in TRAVELLING_STEPS else None
    if trace:
        cmd.append(f"--trace-script={trace}")
    cmd.append(str(game))
    env = dict(os.environ, XDG_CONFIG_HOME=str(config_root))
    timed_out = False
    with stdout.open("wb") as out:
        proc = subprocess.Popen(cmd, stdin=subprocess.DEVNULL, stdout=out, stderr=subprocess.STDOUT,
                                env=env, start_new_session=True)
        try:
            exit_code = proc.wait(timeout=seconds + KILL_GRACE_SECONDS)
        except subprocess.TimeoutExpired:
            timed_out = True
            try:
                os.killpg(proc.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            exit_code = proc.wait()
    return _step_record(name, chain, index, exec_text, trace, exit_code, timed_out,
                        (smoke_log, stdout, engine_log, live_engine_log), portable)


def check_maps(game, chain):
    maps_dir = Path(game) / "Maps"
    if not maps_dir.is_dir():
        sys.exit(f"saveload: no Maps directory in {game}")
    stems = {p.stem.lower() for p in maps_dir.iterdir() if p.suffix.lower() in MAP_SUFFIXES}
    missing = [m for m in chain.maps if m.lower() not in stems]
    if missing:
        sys.exit(f"saveload: {game} has no map {', '.join(missing)} that this chain needs")


def prepare_config_root(config_root):
    """Give the scratch config root the host's Settings.json, so the run picks the same renderer."""
    target = config_root / "SurrealEngine"
    target.mkdir(parents=True, exist_ok=True)
    settings = target / "Settings.json"
    if settings.exists():
        return
    host = Path.home() / ".config" / "SurrealEngine" / "Settings.json"
    if host.is_file():
        shutil.copyfile(host, settings)
    else:
        print(f"saveload: no {host} to copy; the engine will use its default settings", file=sys.stderr)


def write_atomically(path, text):
    tmp = path.with_name(path.name + ".tmp")
    tmp.write_text(text)
    os.replace(tmp, path)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--game", required=True, help="path to the game folder")
    ap.add_argument("--chain", required=True, choices=sorted(CHAINS), help="which game's chain to run")
    ap.add_argument("--user-dir", required=True, help="fresh directory for the engine's game files and saves")
    ap.add_argument("--config-root", required=True,
                    help="XDG_CONFIG_HOME for the engine (settings and the plaintext engine log)")
    ap.add_argument("--out", required=True, help="report JSON path")
    ap.add_argument("--markdown", help="write the markdown report here")
    ap.add_argument("--exe", default=str(REPO_ROOT / "build" / "SurrealEngine"))
    ap.add_argument("--seconds", type=int, default=DEFAULT_SECONDS, help="run time per step")
    ap.add_argument("--logdir", help="directory for the per-step logs (default: <out>.d)")
    ap.add_argument("--portable", action="store_true", help="write a report with no host path in it")
    args = ap.parse_args()

    chain = CHAINS[args.chain]
    user_dir = Path(args.user_dir).resolve()
    config_root = Path(args.config_root).resolve()
    out = Path(args.out)
    logdir = Path(args.logdir) if args.logdir else out.with_name(out.name + ".d")
    if not Path(args.exe).is_file():
        sys.exit(f"saveload: no engine at {args.exe}")
    check_maps(args.game, chain)
    if user_dir.exists() and any(user_dir.iterdir()):
        sys.exit(f"saveload: {user_dir} already holds files; a chain needs a fresh user directory")
    for index in range(len(STEP_NAMES)):
        end = plan_end(step_plan(chain, index))
        if end + SETTLE_SECONDS > args.seconds:
            sys.exit(f"saveload: step {STEP_NAMES[index]} schedules to {end} s, too late for --seconds {args.seconds}")
    user_dir.mkdir(parents=True, exist_ok=True)
    logdir.mkdir(parents=True, exist_ok=True)
    prepare_config_root(config_root)

    steps = []
    report = {"game": PORTABLE_GAME if args.portable else str(args.game), "chain": args.chain,
              "seconds": args.seconds, "steps": steps, "hops": []}
    complete = True
    for index in range(len(STEP_NAMES)):
        step = run_step(args.exe, args.game, chain, index, args.seconds, user_dir, config_root,
                        logdir, args.portable)
        steps.append(step)
        report["hops"] = build_hops(steps)
        # Rewritten after every step so an interrupted chain keeps the steps made.
        write_atomically(out, json.dumps(report, indent=2) + "\n")
        if args.markdown:
            write_atomically(Path(args.markdown), render_markdown(report))
        print(f"{step['step']:13} {step['verdict']:8} exit={step['exit_code']} "
              f"{step['problem'] or 'ok'}", flush=True)
        if step["problem"]:
            complete = False
            break
    print(verdict_sentence(report), flush=True)
    sys.exit(0 if complete else 1)


if __name__ == "__main__":
    main()
