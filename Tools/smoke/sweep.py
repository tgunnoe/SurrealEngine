#!/usr/bin/env python3
"""Run SurrealEngine over the maps of one game and write a JSON report.

Required environment (read from the caller, never set here):
  SURREALWIDGETS_DISPLAY_BACKEND=X11
  LD_LIBRARY_PATH must include the Vulkan loader and /run/opengl-driver/lib,
  e.g. "$(nix-build '<nixpkgs>' -A vulkan-loader --no-out-link)/lib:/run/opengl-driver/lib"

Each map runs in its own engine process with --autostart, --exit-after and
--smoke-log. See README.md for the log grammar and report schema.
"""
import argparse
import json
import os
import re
import shutil
import signal
import subprocess
import sys
import time
from pathlib import Path

# Isolated mode (python3 -I) puts no directory on sys.path.
sys.path.insert(0, str(Path(__file__).resolve().parent))
from triage.matchrule import match_started  # noqa: E402
from triage.scenario import ASCII_UPPER, game_text  # noqa: E402

REPO_ROOT = Path(__file__).resolve().parents[2]
MAP_SUFFIXES = {".unr", ".dx"}
KILL_GRACE_SECONDS = 45
GRIM_TIMEOUT_SECONDS = 10
PORTABLE_GAME = "<game folder>"
# A prefix is one word and a class is Package.Class: nothing in either can add a url option.
PREFIX_WORD = re.compile(r"[A-Za-z0-9_]+")
GAME_CLASS = re.compile(r"[A-Za-z0-9_]+\.[A-Za-z0-9_]+")


def map_subdir_names(names):
    """The --map-subdir values in the order of first mention, each a bare folder name.

    A name is joined onto Maps/ as given, so one holding a separator or standing for the
    current or parent folder would name something other than a folder directly under Maps.
    """
    unique = list(dict.fromkeys(names))
    for name in unique:
        if not name or name in (".", "..") or "/" in name or "\\" in name:
            sys.exit(f"sweep: --map-subdir takes a folder name under Maps, not {name}")
    return unique


def game_by_prefix_entries(entries):
    """The --game-by-prefix values as a {prefix: class} mapping in the order given.

    Each entry is PREFIX=Package.Class. The class goes into the url as ?game=<class>, so it
    must hold nothing that could add an option. Prefixes that differ only in ASCII case name
    one prefix, so a repeat of either spelling is refused.
    """
    mapping = {}
    seen = set()
    for entry in entries:
        prefix, equals, game = entry.partition("=")
        if not (equals and PREFIX_WORD.fullmatch(prefix) and GAME_CLASS.fullmatch(game)):
            sys.exit(f"sweep: --game-by-prefix: bad entry '{entry}' (expected PREFIX=Package.Class)")
        folded = prefix.translate(ASCII_UPPER)
        if folded in seen:
            sys.exit(f"sweep: --game-by-prefix: prefix '{prefix}' given twice")
        seen.add(folded)
        mapping[prefix] = game
    return mapping


def list_maps(game, subdirs=()):
    """The maps under <game>/Maps, then those of each Maps/<subdir> in the order given.

    The engine resolves a map by its bare name, so a stem found in two folders would name
    two maps by one name; the sweep refuses that listing rather than pick one. Stems are
    compared without regard to ASCII case, the only case the engine folds in a name, so
    Bluff and bluff are one name; the refusal names the stem as found.
    """
    maps_dir = Path(game) / "Maps"
    if not maps_dir.is_dir():
        sys.exit(f"sweep: no Maps directory in {game}")
    folders = [maps_dir]
    for name in subdirs:
        folder = maps_dir / name
        if not folder.is_dir():
            sys.exit(f"sweep: no Maps/{name} directory in {game}")
        folders.append(folder)
    maps = []
    folder_of = {}
    for folder in folders:
        for p in sorted((p for p in folder.iterdir() if p.suffix.lower() in MAP_SUFFIXES),
                        key=lambda p: p.name):
            if folder_of.setdefault(p.stem.translate(ASCII_UPPER), folder) != folder:
                sys.exit(f"sweep: map name {p.stem} appears in more than one folder")
            maps.append(p)
    return maps


def parse_log(path):
    info = {"script_errors": 0, "stub_hits": 0, "missing_natives": 0,
            "exceptions": 0, "script_warnings": 0, "fps_avg": None, "fatal_signal": False,
            "summary": False, "maps_loaded": [], "login": False, "match_started": False}
    fps = []
    try:
        lines = Path(path).read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        return info, fps
    for line in lines:
        kind, _, rest = line.partition(" ")
        if kind == "LOGIN":
            info["login"] = True
        elif kind == "SCRIPT_ERROR":
            info["script_errors"] += 1
        elif kind == "SCRIPT_WARNING":
            info["script_warnings"] += 1
        elif kind == "STUB":
            info["stub_hits"] += 1
        elif kind == "MISSING_NATIVE":
            info["missing_natives"] += 1
        elif kind == "EXCEPTION":
            info["exceptions"] += 1
        elif kind == "MAP":
            info["maps_loaded"].append(rest)
        elif kind == "SUMMARY":
            info["summary"] = True
        elif kind == "FPS":
            try:
                fps.append(float(rest))
            except ValueError:
                pass
        elif kind == "FATAL" and rest.startswith("signal"):
            info["fatal_signal"] = True
    info["match_started"] = match_started(lines)
    if fps:
        info["fps_avg"] = sum(fps) / len(fps)
    return info, fps


def classify(info, exit_code, timed_out, requested_map):
    """Verdict for one map run; `info` is the dict parse_log returns.

    Warnings are deliberately absent: whether they count against a clean run is a policy
    for the reader of the report.
    """
    if timed_out:
        return "timeout"
    if info["fatal_signal"]:
        return "crash"
    if exit_code == 0 and info["summary"] and (requested_map not in info["maps_loaded"] or info["fps_avg"] is None):
        return "idle"
    if exit_code == 0 and info["summary"] and not (info["script_errors"] or info["missing_natives"] or info["exceptions"]):
        return "pass"
    return "fail"


def run_map(exe, game, map_path, seconds, logdir, url_options="", exec_text="", portable=False,
            game_text=""):
    name = map_path.stem
    log = logdir / f"{name}.log"
    shot = logdir / f"{name}.png"
    # A child that dies before opening its log must not inherit a previous sweep's files.
    log.unlink(missing_ok=True)
    shot.unlink(missing_ok=True)
    cmd = [str(exe), "--autostart", f"--exit-after={seconds}",
           f"--smoke-log={log}", f"--url={name}{game_text}{url_options}"]
    if exec_text:
        cmd.append(f"--exec={exec_text}")
    cmd.append(str(game))
    proc = subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                            start_new_session=True)
    start = time.monotonic()
    deadline = start + seconds + KILL_GRACE_SECONDS
    shot_at = start + seconds / 2
    screenshot = None
    timed_out = False
    while proc.poll() is None:
        now = time.monotonic()
        if screenshot is None and now >= shot_at and shutil.which("grim"):
            try:
                r = subprocess.run(["grim", str(shot)], capture_output=True, timeout=GRIM_TIMEOUT_SECONDS)
                screenshot = str(shot) if r.returncode == 0 and shot.exists() else None
            except subprocess.TimeoutExpired:
                screenshot = None
            shot_at = float("inf")
        if now >= deadline:
            timed_out = True
            try:
                os.killpg(proc.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            break
        time.sleep(0.2)
    exit_code = proc.wait()

    info, _ = parse_log(log)
    verdict = classify(info, exit_code, timed_out, name)
    return {
        "map": name,
        "exit_code": exit_code,
        "script_errors": info["script_errors"],
        "stub_hits": info["stub_hits"],
        "missing_natives": info["missing_natives"],
        "exceptions": info["exceptions"],
        "script_warnings": info["script_warnings"],
        "fps_avg": info["fps_avg"],
        "maps_loaded": info["maps_loaded"],
        "login": info["login"],
        "match_started": info["match_started"],
        "log": log.name if portable else str(log),
        "screenshot": None if portable else screenshot,
        "verdict": verdict,
    }


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--game", required=True, help="path to the game folder")
    ap.add_argument("--seconds", type=int, default=15, help="run time per map")
    ap.add_argument("--limit", type=int, default=0, help="run only the first N maps (0 = all)")
    ap.add_argument("--out", required=True, help="report JSON path")
    ap.add_argument("--exe", default=str(REPO_ROOT / "build" / "SurrealEngine"))
    ap.add_argument("--logdir", help="directory for per-map logs and screenshots (default: <out>.d)")
    ap.add_argument("--url-options", default="", help="text appended to every --url=<map>, e.g. ?NumBots=4")
    ap.add_argument("--exec", dest="exec_text", default="", help="passed as --exec=<text> to every run")
    ap.add_argument("--portable", action="store_true", help="write a report with no host path in it")
    ap.add_argument("--map-subdir", action="append", default=[], metavar="NAME",
                    help="also sweep the maps under Maps/<name> by their bare names; "
                         "repeat --map-subdir for more folders")
    ap.add_argument("--game-by-prefix", action="append", default=[], metavar="PREFIX=Package.Class",
                    help="run each map named PREFIX-<name> under ?game=Package.Class, placed "
                         "right after the map name; repeat --game-by-prefix for more prefixes")
    args = ap.parse_args()

    out = Path(args.out)
    logdir = Path(args.logdir) if args.logdir else out.with_name(out.name + ".d")
    logdir.mkdir(parents=True, exist_ok=True)

    subdirs = map_subdir_names(args.map_subdir)
    by_prefix = game_by_prefix_entries(args.game_by_prefix)
    maps = list_maps(args.game, subdirs)
    if args.limit > 0:
        maps = maps[:args.limit]

    results = []
    report = {"game": PORTABLE_GAME if args.portable else str(args.game), "seconds": args.seconds,
              "url_options": args.url_options or None, "exec": args.exec_text or None,
              "game_by_prefix": by_prefix or None, "map_subdirs": subdirs or None, "maps": results}
    for m in maps:
        entry = run_map(args.exe, args.game, m, args.seconds, logdir,
                        args.url_options, args.exec_text, args.portable,
                        game_text=game_text(by_prefix.items(), m.stem))
        print(f"{entry['verdict']:8} {entry['map']}", flush=True)
        results.append(entry)
        # Rewritten after every map so an interrupted sweep keeps its rows.
        tmp = out.with_name(out.name + ".tmp")
        tmp.write_text(json.dumps(report, indent=2) + "\n")
        os.replace(tmp, out)


if __name__ == "__main__":
    main()
