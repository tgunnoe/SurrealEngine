#!/usr/bin/env python3
"""The twelve mover and zone pins: their recipes, and the judges that read a run against them.

A pin is a run whose outcome was recorded once (a count of script calls, a distance, a height)
and is read again later to see whether a change moved it. The table below holds each pin's
engine arguments; `judge` reads the run's smoke log and engine log and prints one verdict:

  PIN-HELD    the recorded outcome was reproduced
  PIN-BROKEN  the run completed and the outcome differs
  VOID        the run cannot be read (no summary, no frames, no probe, no unique player name)

The player's object name is read from the run, never written here: the smoke log's LOGIN line
carries none, so the name is the one object the engine log's position probes
(`call: class:<Class> resolved to <Name> at (x, y, z)`) answer for the player's class. A bot of
the same class can answer the same probe; the player is then the one whose first answer after
the `place:` line agrees with the placed point.

Command line:
  pinjudge.py list
  pinjudge.py cmd <pin>                          the engine arguments, one shell-quoted line
  pinjudge.py judge <pin> <smoke log> <engine log>
"""
import math
import re
import shlex
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import NamedTuple

# An engine option no binary knows is ignored, and the pins were recorded with no frame limit.
FRAME_LIMIT_ARG = "--frame-limit=0"
PLAYER_CLASS = "UPak.UPakFemaleOne"
# How far a probe may be from the placed point in x and y and still be the placed player.
PLACE_AGREES_WITHIN = 64.0
HELD, BROKEN, VOID = "PIN-HELD", "PIN-BROKEN", "VOID"


def seconds(t):
    """A schedule time as the --exec grammar writes it: no trailing zeros."""
    return f"{t:.3f}".rstrip("0").rstrip(".")


def probes(forms, start, stop, step):
    """One `call LineOfSightTo` item per form every `step` seconds from `start` to `stop`."""
    items = []
    n = round((stop - start) / step)
    for i in range(n + 1):
        t = start + i * step
        items += [f"{seconds(t)}:call LineOfSightTo {form}" for form in forms]
    return items


def schedule(*parts):
    """The --exec text of the items given, in time order and, at one time, in the order written.

    A part is one item or a list of items; an item that holds several `;`-joined items is split.
    """
    items = []
    for part in parts:
        for chunk in ([part] if isinstance(part, str) else part):
            items += chunk.split(";")
    items.sort(key=lambda item: float(item.partition(":")[0]))
    return ";".join(items)


@dataclass(frozen=True)
class Pin:
    id: str
    game: str  # unreal, ut or deusex: which game folder and config root the run uses
    url: str  # the map and game text; `?Class=` is added for unreal
    exec: str
    trace: str
    exit_after: int
    player_class: str  # the probe's class word, whose answer names the player
    judge: str
    params: dict

    def full_url(self):
        return self.url + (f"?Class={PLAYER_CLASS}" if self.game == "unreal" else "")

    def args(self, smoke_log="<smoke log>"):
        """Everything after the executable and before the game folder."""
        return ["--autostart", f"--exit-after={self.exit_after}", f"--smoke-log={smoke_log}",
                FRAME_LIMIT_ARG, f"--url={self.full_url()}", f"--exec={self.exec}",
                f"--trace-script={self.trace}"]


# Where each recipe places the player. Unreal Gold and Deus Ex: the first STATE line's location of
# the map without a placement, read from one 8 s run, at 4 s. Unreal Tournament: the start the
# choreography was recorded from, 0.5 s after the Fire that starts the match (a place before it is
# undone by the respawn).
START_UG = "place -2134.6 -89.3 -384.0 128"
START_UT = "5.5:place 909.3 1916.3 -634.6 270"
START_DX = "place -1149.2 825.8 -65.1 0"
START_AT = "4:"

UG = "class:UPakFemaleOne"
UT = "class:TMale2"
DX = "class:JCDentonMale"

ENC_TRACE = ("Mover.Bump,Mover.EncroachingOn,Mover.MakeGroupReturn,Mover.MakeGroupStop,Pawn.EncroachedBy,"
             "Pawn.TakeDamage,Pawn.KilledBy,Pawn.gibbedBy")
SLIDE_TRACE = "Mover.Bump,Mover.Trigger,Actor.HitWall"
WARP_TRACE = ("WarpZoneInfo.ActorEntered,WarpZoneInfo.ActorLeaving,WarpZoneInfo.Warp,WarpZoneInfo.UnWarp,"
              "ZoneInfo.ActorEntered,Pawn.Died")
NUCLEUS_URL = "CTF-Nucleus?game=Botpack.CTFGame"


def nucleus(pin_id, point, judge_params):
    x, y = point
    return Pin(pin_id, "ut", NUCLEUS_URL,
               schedule("5:Fire", f"8:place {x} {y} 444", probes([UT], 7.75, 16.25, 0.25)),
               "Pawn.TakeDamage,Pawn.Died", 40, "TMale2", "nucleus", judge_params)


def pit(pin_id, place, entry, far, column):
    return Pin(pin_id, "ut", "DM-Fractal",
               schedule("5:Fire", f"8:place {place}", probes([UT], 7, 14, 0.5)),
               WARP_TRACE, 16, "TMale2", "warp", {"entry": entry, "far": far, "column": column})


PIN_LIST = [
    Pin("mv-proj-ug", "unreal", "Vortex2",
        schedule(START_AT + START_UG,
                 "5:Summon UnrealShare.DispersionPistol;5.3:hold axis aForward 300 1.5;7:SwitchWeapon 1;"
                 "7.2:hold axis aForward 37 1.7;7.2:hold axis aStrafe -298 1.7;9.2:CauseEvent litefall3;"
                 "11:Fire;11.8:Fire",
                 probes([UG, "name:Mover44"], 5, 14, 0.25)),
        "Projectile.HitWall,Projectile.Explode,Projectile.Touch,Projectile.ProcessTouch,Mover.Bump,"
        "Mover.TakeDamage,Mover.Trigger,Weapon.ProjectileFire,PlayerPawn.Fire",
        17, "UPakFemaleOne", "projectile", {"mover": "Mover44"}),
    Pin("mv-proj-ut", "ut", "DM-Deck16][",
        schedule(START_UT,
                 "5:Fire;6:Summon Botpack.PulseGun;6.5:hold axis aForward 300 1.0;8.5:SwitchWeapon 5;9:Ghost;"
                 "9.1:hold axis aLookUp -512 4;9.2:hold axis aStrafe 300 0.54;13.2:Fire;13.8:Fire",
                 probes([UT, "name:Mover0"], 6, 15, 0.25)),
        "Projectile.HitWall,Projectile.Explode,Projectile.Touch,Projectile.ProcessTouch,Mover.Bump,"
        "Mover.TakeDamage,Weapon.ProjectileFire,PlayerPawn.Fire,Inventory.Touch",
        18, "TMale2", "projectile", {"mover": "Mover0"}),
    Pin("mv-slide-ug", "unreal", "Vortex2",
        schedule(START_AT + START_UG,
                 "5:hold axis aForward 289 2.7;5:hold axis aStrafe -80 2.7;9:CauseEvent litefall3;"
                 "10.4:hold axis aForward -298 2.5;10.4:hold axis aStrafe -35 2.5",
                 probes([UG, "name:Mover44"], 5, 16, 0.25)),
        SLIDE_TRACE, 17, "UPakFemaleOne", "slide", {"form": UG, "t0": 11.5, "t1": 13.0, "min": 100.0}),
    Pin("mv-slide-dx", "deusex", "00_Training",
        schedule(START_AT + START_DX,
                 "5:hold axis aForward -212 3;5:hold axis aStrafe 212 3",
                 probes([DX, "name:DeusExMover49", "name:DeusExMover32"], 4.5, 9, 0.25)),
        "Mover.Bump,Mover.Trigger,Mover.EncroachingOn", 11, "JCDentonMale", "slide",
        {"form": DX, "t0": 5.75, "t1": 9.0, "min": 15.0}),
    Pin("mv-enc-ug", "unreal", "Vortex2",
        schedule(START_AT + START_UG,
                 "5:hold axis aForward 300 3.2;10:hold axis aForward -239 2.6;10:hold axis aStrafe -181 2.6;"
                 "14:CauseEvent litefall3",
                 probes([UG], 5, 22, 0.5)),
        ENC_TRACE, 26, "UPakFemaleOne", "crush", {}),
    Pin("mv-enc-ut", "ut", "DM-Deck16][",
        schedule(START_UT,
                 "5:Fire;6:Ghost;6.5:hold axis aStrafe 300 0.54;7.2:hold axis aUp -300 0.88;8.3:Walk;"
                 "10.3:Ghost;10.6:hold axis aUp -300 0.8;11.8:Walk",
                 probes([UT], 6, 26, 0.25)),
        ENC_TRACE, 30, "TMale2", "lift", {"returns": 3}),
    Pin("mv-enc-dx", "deusex", "00_Training",
        schedule(START_AT + START_DX,
                 "5:hold axis aForward 300 6;11.2:call FrobTarget.Frob self none;13.5:hold axis aForward 300 0.6;"
                 "15:hold axis aTurn -2633 1;17.2:call FrobTarget.Frob self none",
                 probes([DX], 5, 26, 0.5)),
        "Mover.Trigger,Mover.EncroachingOn,Mover.MakeGroupReturn,Mover.MakeGroupStop,Pawn.EncroachedBy,"
        "Pawn.TakeDamage,Pawn.KilledBy,Pawn.gibbedBy,DeusExMover.Frob,DeusExMover.Trigger",
        30, "JCDentonMale", "doors", {"low": 974, "high": 1460, "apart": 2}),
    pit("zn-w1", "1186.8 -454.5 629.9", "WarpZoneInfo2", "WarpZoneInfo3", (786.8, -446.5)),
    pit("zn-w2", "-577.6 -441.0 630.55", "WarpZoneInfo4", "WarpZoneInfo5", (-177.6, -449.0)),
    nucleus("zn-ctf1", ("28.0", "3516.0"), {"kind": "stands"}),
    nucleus("zn-ctf2", ("29.3", "3505.2"), {"kind": "stands"}),
    nucleus("zn-ctf3", ("-16.0", "3504.0"), {"kind": "falls"}),
]
PINS = {p.id: p for p in PIN_LIST}


class Probe(NamedTuple):
    t: float
    form: str
    name: str
    at: tuple


class Trace(NamedTuple):
    t: float
    obj: str
    func: str
    args: str


class Run(NamedTuple):
    smoke: list
    answers: list
    traces: list
    places: list  # (t, (x, y, z) left at)


PROBE_LINE = re.compile(r"^\[(?P<t>[\d.]+)\] .*?call: (?P<form>\S+) resolved to (?P<name>\S+) at "
                        r"\((?P<x>-?[\d.]+), (?P<y>-?[\d.]+), (?P<z>-?[\d.]+)\)")
TRACE_LINE = re.compile(r"^\[(?P<t>[\d.]+)\] .*?trace: (?P<obj>\S+) \S+ \S+: (?P<func>\w+)\((?P<args>.*)\)\s*$")
PLACE_LINE = re.compile(r"^\[(?P<t>[\d.]+)\] .*?place: PLACE \(.*?\) -> \((?P<x>-?[\d.]+), (?P<y>-?[\d.]+), "
                        r"(?P<z>-?[\d.]+)\)")


def read_run(smoke_text, engine_text):
    answers, traces, places = [], [], []
    for line in engine_text.splitlines():
        if m := PROBE_LINE.match(line):
            answers.append(Probe(float(m["t"]), m["form"], m["name"],
                                 (float(m["x"]), float(m["y"]), float(m["z"]))))
        elif m := TRACE_LINE.match(line):
            traces.append(Trace(float(m["t"]), m["obj"], m["func"], m["args"]))
        elif m := PLACE_LINE.match(line):
            places.append((float(m["t"]), (float(m["x"]), float(m["y"]), float(m["z"]))))
    return Run(smoke_text.splitlines(), answers, traces, places)


def player_name(run, class_word):
    """The player's object name from the probe answers for `class:<class_word>`, or None.

    One object answering is the player. Two (a bot of the class) are told apart by the
    placement: the player is the one whose first answer after the `place:` line agrees with
    the placed point in x and y. With no unique answer there is no name.
    """
    mine = [a for a in run.answers if a.form == f"class:{class_word}"]
    names = list(dict.fromkeys(a.name for a in mine))
    if len(names) == 1:
        return names[0]
    if len(names) < 2 or not run.places:
        return None
    placed_t, (px, py, _) = run.places[0]
    firsts = {}
    for a in mine:
        if a.t >= placed_t:
            firsts.setdefault(a.name, a)
    near = [n for n, a in firsts.items() if math.hypot(a.at[0] - px, a.at[1] - py) <= PLACE_AGREES_WITHIN]
    return near[0] if len(near) == 1 else None


def exec_items(exec_text):
    items = []
    for item in exec_text.split(";"):
        t, _, command = item.partition(":")
        items.append((float(t), command))
    return items


def probe_at(pin, run, form, t):
    """The answer of the probe the pin schedules at time `t` for `form`, matched by order."""
    scheduled = [at for at, command in exec_items(pin.exec) if command == f"call LineOfSightTo {form}"]
    got = [a for a in run.answers if a.form == form]
    if len(got) != len(scheduled) or t not in scheduled:
        return None
    return got[scheduled.index(t)]


def count(run, func, args=None, obj=None):
    return sum(1 for e in run.traces
               if e.func == func and (args is None or e.args == args) and (obj is None or e.obj == obj))


def void_reason(run):
    if not any(line.startswith("SUMMARY") for line in run.smoke):
        return "no SUMMARY line"
    fps = [float(line.split()[1]) for line in run.smoke if line.startswith("FPS ") and line.split()[1:]]
    if any(f < 1 for f in fps):
        return "an FPS line below 1"
    if not run.answers:
        return "no probe answer"
    return None


def judge_projectile(pin, run, name):
    mover = pin.params["mover"]
    hits = sum(1 for e in run.traces if e.func == "HitWall" and e.args.split(", ")[-1] == mover)
    bumps = count(run, "Bump")
    touches = count(run, "Touch", mover)
    damage = count(run, "TakeDamage", obj=mover)
    ok = hits >= 2 and bumps >= 2 and touches == 0 and damage == 0
    return (HELD if ok else BROKEN), [f"HitWall {hits} Bump {bumps} Touch {touches} TakeDamage {damage}"]


def judge_slide(pin, run, name):
    p = pin.params
    first, last = (probe_at(pin, run, p["form"], p["t0"]), probe_at(pin, run, p["form"], p["t1"]))
    if first is None or last is None:
        return VOID, [f"the probes at {p['t0']} s and {p['t1']} s are not in the log"]
    moved = math.hypot(last.at[0] - first.at[0], last.at[1] - first.at[1])
    return (HELD if moved >= p["min"] else BROKEN), [f"moved {moved:.1f} (at least {p['min']:g})"]


def judge_crush(pin, run, name):
    hits = [i for i, e in enumerate(run.traces) if e.func == "EncroachingOn" and e.args == name]
    killed = any(e.func == "KilledBy" and e.args == name for e in run.traces[hits[0]:]) if hits else False
    ok = len(hits) == 1 and killed
    return (HELD if ok else BROKEN), [f"EncroachingOn({name}) {len(hits)} followed by KilledBy {killed}"]


def judge_lift(pin, run, name):
    hits = [i for i, e in enumerate(run.traces) if e.func == "EncroachingOn" and e.args == name]
    ends = hits[1:] + [len(run.traces)]
    returned = [any(e.func == "MakeGroupReturn" for e in run.traces[i:end]) for i, end in zip(hits, ends)]
    ok = len(hits) == pin.params["returns"] and all(returned)
    return (HELD if ok else BROKEN), [f"EncroachingOn({name}) {len(hits)}, each with MakeGroupReturn: {all(returned)}"]


def judge_doors(pin, run, name):
    p = pin.params
    crushed = count(run, "EncroachingOn", name)
    by = count(run, "EncroachedBy")
    ok = all(p["low"] <= n <= p["high"] for n in (crushed, by)) and abs(crushed - by) <= p["apart"]
    return (HELD if ok else BROKEN), [f"EncroachingOn({name}) {crushed} EncroachedBy {by}"]


def entries(run, zone, name):
    """The entries of `name` into `zone`: the two lines a call through Super prints are one entry."""
    found = []
    for e in run.traces:
        if e.func == "ActorEntered" and e.obj == zone and e.args == name:
            if not found or (found[-1].t, found[-1].args) != (e.t, e.args):
                found.append(e)
    return found


def judge_warp(pin, run, name):
    p = pin.params
    here, far = entries(run, p["entry"], name), entries(run, p["far"], name)
    if not here:
        return BROKEN, [f"no entry on {p['entry']}"]
    start = here[0].t
    window = [e for e in run.traces if start <= e.t <= start + SECOND_ENTRY_WITHIN]
    unwarp = sum(1 for e in window if e.func == "UnWarp")
    warp = [e for e in window if e.func == "Warp"]
    second = [e for e in here[1:] if e.t < start + SECOND_ENTRY_WITHIN]
    after = [a for a in run.answers if a.form == f"class:{pin.player_class}" and warp and a.t > warp[0].t]
    gap = math.hypot(after[0].at[0] - p["column"][0], after[0].at[1] - p["column"][1]) if after else None
    ok = (not far and not second and unwarp == 1 and len(warp) == 1 and gap is not None
          and gap <= COLUMN_WITHIN)
    shown = "no probe after the warp" if gap is None else f"first probe {gap:.1f} from the column"
    return (HELD if ok else BROKEN), [f"entries {len(here)} far {len(far)} UnWarp {unwarp} Warp {len(warp)} "
                                      f"second entry within {SECOND_ENTRY_WITHIN:g} s {len(second)}; {shown}"]


def judge_nucleus(pin, run, name):
    if not run.places:
        return VOID, ["no place: line"]
    placed = run.places[0][0]
    zs = [(a.t - placed, a.at[2]) for a in run.answers if a.form == f"class:{pin.player_class}" and a.t >= placed]
    if not zs:
        return VOID, ["no probe after the placement"]
    if pin.params["kind"] == "stands":
        late = [z for t, z in zs if t >= STAND_BY]
        ok = bool(late) and all(abs(z - STAND_Z) <= STAND_WITHIN for z in late)
        return (HELD if ok else BROKEN), [f"z of the probes from {STAND_BY:g} s: {sorted(set(late))[:3]}"]
    low = min(z for _, z in zs)
    ok = low < SLAB_TOP and zs[-1][1] <= FALL_SETTLES_AT
    return (HELD if ok else BROKEN), [f"lowest z {low:.1f}, last z {zs[-1][1]:.1f}"]


# The recorded bands: the Nucleus slab's top is z 384 and a standing player's centre z 424.
SECOND_ENTRY_WITHIN = 5.0
COLUMN_WITHIN = 12.0
STAND_Z, STAND_WITHIN, STAND_BY = 424.0, 2.0, 1.5
SLAB_TOP, FALL_SETTLES_AT = 384.0, 100.0

JUDGES = {"projectile": judge_projectile, "slide": judge_slide, "crush": judge_crush, "lift": judge_lift,
          "doors": judge_doors, "warp": judge_warp, "nucleus": judge_nucleus}


def judge(pin, smoke_text, engine_text):
    """(verdict, detail lines) of one run against `pin`."""
    run = read_run(smoke_text, engine_text)
    reason = void_reason(run)
    if reason:
        return VOID, [reason]
    name = player_name(run, pin.player_class)
    if name is None:
        return VOID, [f"no unique player name from the class:{pin.player_class} probes"]
    verdict, detail = JUDGES[pin.judge](pin, run, name)
    return verdict, [f"player {name}"] + detail


def main(argv=None):
    argv = sys.argv[1:] if argv is None else argv
    if argv[:1] == ["list"]:
        for pin in PINS.values():
            print(f"{pin.id}\t{pin.game}\t{pin.url}\t{pin.exit_after}\t{pin.judge}")
        return 0
    if len(argv) == 2 and argv[0] == "cmd" and argv[1] in PINS:
        print(" ".join(shlex.quote(a) for a in PINS[argv[1]].args()))
        return 0
    if len(argv) == 4 and argv[0] == "judge" and argv[1] in PINS:
        try:
            smoke = Path(argv[2]).read_text(encoding="utf-8", errors="replace")
            engine = Path(argv[3]).read_text(encoding="utf-8", errors="replace")
        except OSError as e:
            sys.exit(f"pinjudge: cannot read a log: {e}")
        verdict, detail = judge(PINS[argv[1]], smoke, engine)
        print(f"{verdict} {argv[1]}")
        for line in detail:
            print(f"  {line}")
        return {HELD: 0, BROKEN: 1}.get(verdict, 2)
    sys.exit(__doc__)


if __name__ == "__main__":
    sys.exit(main())
