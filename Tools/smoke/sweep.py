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
import shutil
import signal
import subprocess
import sys
import time
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
MAP_SUFFIXES = {".unr", ".dx"}
KILL_GRACE_SECONDS = 45


def list_maps(game):
    maps_dir = Path(game) / "Maps"
    if not maps_dir.is_dir():
        sys.exit(f"sweep: no Maps directory in {game}")
    return sorted(
        (p for p in maps_dir.iterdir() if p.suffix.lower() in MAP_SUFFIXES),
        key=lambda p: p.name,
    )


def parse_log(path):
    info = {"script_errors": 0, "stub_hits": 0, "missing_natives": 0,
            "exceptions": 0, "fps_avg": None, "fatal_signal": False}
    fps = []
    try:
        lines = Path(path).read_text(errors="replace").splitlines()
    except OSError:
        return info, fps
    for line in lines:
        kind, _, rest = line.partition(" ")
        if kind == "SCRIPT_ERROR":
            info["script_errors"] += 1
        elif kind == "STUB":
            info["stub_hits"] += 1
        elif kind == "MISSING_NATIVE":
            info["missing_natives"] += 1
        elif kind == "EXCEPTION":
            info["exceptions"] += 1
        elif kind == "FPS":
            try:
                fps.append(float(rest))
            except ValueError:
                pass
        elif kind == "FATAL" and rest.startswith("signal"):
            info["fatal_signal"] = True
    if fps:
        info["fps_avg"] = sum(fps) / len(fps)
    return info, fps


def run_map(exe, game, map_path, seconds, logdir):
    name = map_path.stem
    log = logdir / f"{name}.log"
    shot = logdir / f"{name}.png"
    cmd = [str(exe), "--autostart", f"--exit-after={seconds}",
           f"--smoke-log={log}", f"--url={name}", str(game)]
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
            r = subprocess.run(["grim", str(shot)], capture_output=True)
            screenshot = str(shot) if r.returncode == 0 and shot.exists() else None
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
    if timed_out:
        verdict = "timeout"
    elif info["fatal_signal"]:
        verdict = "crash"
    elif exit_code == 0 and not (info["script_errors"] or info["missing_natives"] or info["exceptions"]):
        verdict = "pass"
    else:
        verdict = "fail"
    return {
        "map": name,
        "exit_code": exit_code,
        "script_errors": info["script_errors"],
        "stub_hits": info["stub_hits"],
        "missing_natives": info["missing_natives"],
        "exceptions": info["exceptions"],
        "fps_avg": info["fps_avg"],
        "log": str(log),
        "screenshot": screenshot,
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
    args = ap.parse_args()

    out = Path(args.out)
    logdir = Path(args.logdir) if args.logdir else out.with_name(out.name + ".d")
    logdir.mkdir(parents=True, exist_ok=True)

    maps = list_maps(args.game)
    if args.limit > 0:
        maps = maps[:args.limit]

    results = []
    for m in maps:
        entry = run_map(args.exe, args.game, m, args.seconds, logdir)
        print(f"{entry['verdict']:8} {entry['map']}", flush=True)
        results.append(entry)

    report = {"game": str(args.game), "seconds": args.seconds, "maps": results}
    out.write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    main()
