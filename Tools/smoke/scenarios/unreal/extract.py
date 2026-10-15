#!/usr/bin/env python3
"""Extract stub and error sidecars from a directory of smoke logs.

usage: python3 -I extract.py <logdir> [<outdir>]

Reads <logdir>/<map>.log (grammar in Tools/smoke/README.md) and writes
stubs.json and errors.json into <outdir> (default: this script's folder).

Native call stacks are cut from SCRIPT_ERROR text: they hold process
addresses and local build paths that differ on every run and machine,
while the script call stack before them is the stable part.
"""
import json
import os
import re
import sys

ERROR_KINDS = ("SCRIPT_ERROR", "MISSING_NATIVE", "EXCEPTION", "FATAL")
MAX_ERRORS = 5

# Owner classes of STUB text, by the class before the first '.'.
# Gameplay set follows the triage definition: Actor and its gameplay
# subclasses (Pawn, PlayerPawn, Inventory, Weapon, Projectile, Decoration,
# Mover, Trigger, NavigationPoint, GameInfo, LevelInfo, ZoneInfo, Canvas/HUD).
# Matching is by exact class name or by a base-class suffix below.
GAMEPLAY_EXACT = {
    "Actor", "Pawn", "PlayerPawn", "Inventory", "Weapon", "Projectile",
    "Decoration", "Mover", "Trigger", "NavigationPoint", "GameInfo",
    "LevelInfo", "ZoneInfo", "Canvas", "HUD", "Brush", "Light", "Pickup",
    "Carcass", "Effects", "Decal", "Spawnpoint", "PlayerStart", "Scout",
    "Teleporter", "Triggers", "Keypoint", "Info", "Ammo", "Bots",
    "Scoreboard", "Console", "Player", "Viewport", "PlayerReplicationInfo",
    "GameReplicationInfo", "ReplicationInfo", "SkyZoneInfo", "WarpZoneInfo",
    "Counter", "Dispatcher", "Pawns", "Mutator",
}
GAMEPLAY_SUFFIXES = ("Pawn", "Weapon", "Projectile", "Decoration", "Mover",
                     "Trigger", "Zone", "HUD")
NONGAMEPLAY_EXACT = {
    "StatLog", "StatLogFile", "UdpBeacon", "UdpLink", "InternetLink",
    "TcpLink", "IpDrv", "UWebServer", "WebServer", "Commandlet", "Engine",
    "Object", "Palette", "Texture", "Music", "Sound", "Font", "Mesh",
    "LodMesh", "Model", "Level", "Polys", "Bitmap", "Time", "Stats",
}


def owner_of(text):
    head = text.split(None, 1)[0] if text.strip() else ""
    if "." not in head:
        return ""
    return head.split(".", 1)[0]


def classify(owner):
    """True if gameplay, False if non-gameplay, None if undecided.

    Three-way on purpose: an owner in neither set is None, never False,
    so an unknown class cannot make a map look clean.
    """
    if not owner:
        return None
    if owner in NONGAMEPLAY_EXACT:
        return False
    if owner in GAMEPLAY_EXACT or owner.endswith(GAMEPLAY_SUFFIXES):
        return True
    return None


def clean_error(kind, text):
    text = re.sub(r"\s*Native call stack:.*$", "", text).strip()
    return text


def norm(text):
    return re.sub(r"0x[0-9a-fA-F]+", "0x?", text)


def parse(path):
    stubs, errors = [], []
    section = None
    summary_seen = False
    fatal_after_summary = False
    with open(path, encoding="utf-8", errors="replace") as f:
        for raw in f:
            line = raw.rstrip("\n")
            kind, _, text = line.partition(" ")
            if kind == "MAP":
                section = text
            elif kind == "SUMMARY":
                summary_seen = True
            elif kind == "STUB":
                stubs.append(text)
            elif kind in ERROR_KINDS:
                if kind == "FATAL" and summary_seen:
                    fatal_after_summary = True
                errors.append({"kind": kind, "text": clean_error(kind, text),
                               "section": section})
    return stubs, errors, fatal_after_summary


def main(argv):
    if len(argv) not in (2, 3):
        sys.exit(__doc__)
    logdir = argv[1]
    outdir = argv[2] if len(argv) == 3 else os.path.dirname(os.path.abspath(__file__))
    stubs_out, errors_out = {}, {}
    for name in sorted(os.listdir(logdir)):
        if not name.endswith(".log"):
            continue
        m = name[:-4]
        stubs, errors, fas = parse(os.path.join(logdir, name))
        seen, distinct = set(), []
        for s in stubs:
            if s not in seen:
                seen.add(s)
                distinct.append(s)
        stubs_out[m] = [{"text": s, "owner": owner_of(s),
                         "gameplay": classify(owner_of(s))} for s in distinct]
        seen, first = set(), []
        for e in errors:
            key = (e["kind"], e["text"])
            if key in seen:
                continue
            seen.add(key)
            if len(first) < MAX_ERRORS:
                first.append(e)
        errors_out[m] = {"errors": first, "fatal_after_summary": fas}
    os.makedirs(outdir, exist_ok=True)
    for fname, data in (("stubs.json", stubs_out), ("errors.json", errors_out)):
        with open(os.path.join(outdir, fname), "w") as f:
            json.dump(data, f, indent=1, sort_keys=True)
            f.write("\n")


if __name__ == "__main__":
    main(sys.argv)
