"""Extract per-map stub and error sidecars from smoke logs.

usage: python3 -I extract.py <logdir> <outdir>

Writes <outdir>/stubs.json and <outdir>/errors.json. Logs are not committed,
so these sidecars carry the evidence the triage and its checker need.
"""
import json
import os
import sys

ERROR_KINDS = ("SCRIPT_ERROR", "MISSING_NATIVE", "EXCEPTION", "FATAL")

# Engine classes whose subclasses are gameplay (Actor tree minus tools).
GAMEPLAY = {
    "Actor", "Pawn", "PlayerPawn", "Inventory", "Weapon", "Projectile",
    "Decoration", "Mover", "Trigger", "NavigationPoint", "GameInfo",
    "LevelInfo", "ZoneInfo", "Canvas", "HUD", "Brush", "Light", "Pickup",
    "Ammo", "Pawn", "Bot", "Scout", "Spawnpoint", "PlayerStart",
    "Triggers", "Effects", "Carcass", "Teleporter", "Keypoint", "Info",
    "Decal", "Console", "PlayerReplicationInfo", "ReplicationInfo",
    "GameReplicationInfo", "Spectator", "Mutator", "Projector",
    "ScriptedPawn", "Weapons", "ParticleIterator", "Inventory",
}
# Owners that are not gameplay: logging, admin, demo, editor, debug tools.
NON_GAMEPLAY = {
    "DebugInfo", "StatLog", "StatLogFile", "UdpLink", "TcpLink",
    "InternetLink", "UWebServer", "Commandlet", "Engine", "Object",
    "Subsystem", "Player", "Viewport", "NetConnection", "DemoRecDriver",
    "NetDriver", "Palette", "Texture", "Font", "Sound", "Music",
    "FileManager", "Time", "ConsoleCommand",
}


def owner_of(text, bare_owners):
    head = text.split(" ", 1)[0]
    if "." in head:
        return head.split(".", 1)[0]
    name = head.split("(", 1)[0]
    return bare_owners.get(name)


def parse(path):
    stubs, errors, seen_summary, fatal_after = [], [], False, False
    for line in open(path, errors="replace"):
        line = line.rstrip("\n")
        kind, _, text = line.partition(" ")
        if kind == "SUMMARY":
            seen_summary = True
        elif kind == "STUB":
            if text not in stubs:
                stubs.append(text)
        elif kind in ERROR_KINDS:
            if kind == "FATAL" and seen_summary:
                fatal_after = True
            entry = kind + " " + text
            if entry not in errors:
                errors.append(entry)
    return stubs, errors[:5], fatal_after


def main():
    logdir, outdir = sys.argv[1], sys.argv[2]
    bare_owners = {}
    bo = os.path.join(os.path.dirname(os.path.abspath(__file__)), "bare_owners.json")
    if os.path.exists(bo):
        bare_owners = json.load(open(bo))
    stubs_out, errors_out = {}, {}
    for fn in sorted(os.listdir(logdir)):
        if not fn.endswith(".log"):
            continue
        m = fn[:-4]
        stubs, errors, fatal_after = parse(os.path.join(logdir, fn))
        rows = []
        for s in stubs:
            o = owner_of(s, bare_owners)
            rows.append({"text": s, "owner": o,
                         "gameplay": (o in GAMEPLAY) if o else None})
        stubs_out[m] = rows
        errors_out[m] = {"first": errors, "fatal_after_summary": fatal_after}
    json.dump(stubs_out, open(os.path.join(outdir, "stubs.json"), "w"), indent=1, sort_keys=True)
    json.dump(errors_out, open(os.path.join(outdir, "errors.json"), "w"), indent=1, sort_keys=True)


if __name__ == "__main__":
    main()
