"""Extract per-map STUB and error sidecars from a directory of smoke logs.

usage: python3 -I extract.py <logdir> <outdir>

Writes <outdir>/stubs.json and <outdir>/errors.json. See Tools/smoke/README.md
for the log grammar.
"""
import json
import os
import re
import sys

ERROR_KINDS = ("SCRIPT_ERROR", "MISSING_NATIVE", "EXCEPTION", "FATAL")

# Owner class -> superclass chain, read from the export tables of the Deus Ex
# 1112fm packages (Engine.u, DeusEx.u, Core.u): each class export carries its
# super class. Classification needs the chain because a class counts as
# gameplay when it descends from one of GAMEPLAY_ROOTS.
HIERARCHY = {
    "Actor": ["Actor", "Object"],
    "DumpLocation": ["DumpLocation", "Object"],
    "LevelInfo": ["LevelInfo", "ZoneInfo", "Info", "Actor", "Object"],
    "Pawn": ["Pawn", "Actor", "Object"],
    "PlayerPawn": ["PlayerPawn", "Pawn", "Actor", "Object"],
}

GAMEPLAY_ROOTS = {
    "Actor", "Pawn", "PlayerPawn", "Inventory", "Weapon", "Projectile",
    "Decoration", "Mover", "Trigger", "NavigationPoint", "GameInfo",
    "LevelInfo", "ZoneInfo", "Canvas", "HUD",
}


def classify(owner):
    """Return (class, reason) for a stub owner; class is gameplay,
    non-gameplay or unclassified."""
    chain = HIERARCHY.get(owner)
    if chain is None:
        return "unclassified", "owner not in HIERARCHY; extend the table"
    hit = [c for c in chain if c in GAMEPLAY_ROOTS]
    if hit:
        return "gameplay", "%s descends from %s" % (owner, hit[0])
    return "non-gameplay", "%s descends only from %s" % (owner, " > ".join(chain))


# A native backtrace frame: "at <binary>(+0xOFF) [0xADDR]". Frames carry
# build paths and per-run addresses, so the message is cut at the first one.
NATIVE_FRAME = re.compile(r"\s+at \S*\(\S*\+0x[0-9a-fA-F]+\) \[0x[0-9a-fA-F]+\]")


def strip_native_stack(text):
    m = NATIVE_FRAME.search(text)
    if not m:
        return text
    text = text[:m.start()].rstrip()
    if text.endswith("Native call stack:"):
        text = text[:-len("Native call stack:")].rstrip()
    return text


def parse(path):
    stubs, errors, section = [], [], None
    seen_summary = False
    fatal_after_summary = False
    with open(path, encoding="utf-8", errors="replace") as f:
        for raw in f:
            line = raw.rstrip("\n")
            kind, _, text = line.partition(" ")
            if kind in ERROR_KINDS:
                text = strip_native_stack(text)
            if kind == "MAP":
                section = text
            elif kind == "SUMMARY":
                seen_summary = True
            elif kind == "STUB":
                if text not in stubs:
                    stubs.append(text)
            elif kind in ERROR_KINDS:
                if kind == "FATAL" and seen_summary:
                    fatal_after_summary = True
                if not any(e["text"] == text and e["kind"] == kind for e in errors):
                    errors.append({"kind": kind, "text": text, "map_section": section})
    return stubs, errors[:5], fatal_after_summary


def main(logdir, outdir):
    stubs_out, errors_out, owners = {}, {}, {}
    for name in sorted(os.listdir(logdir)):
        if not name.endswith(".log"):
            continue
        m = name[:-4]
        stubs, errors, fas = parse(os.path.join(logdir, name))
        rows = []
        for s in stubs:
            owner = s.split(".", 1)[0] if "." in s else ""
            cls, reason = classify(owner)
            owners[owner] = {"class": cls, "reason": reason}
            rows.append({"text": s, "owner": owner, "class": cls})
        stubs_out[m] = rows
        errors_out[m] = {"errors": errors, "fatal_after_summary": fas}
    with open(os.path.join(outdir, "stubs.json"), "w") as f:
        json.dump({"owners": owners, "maps": stubs_out}, f, indent=1, sort_keys=True)
        f.write("\n")
    with open(os.path.join(outdir, "errors.json"), "w") as f:
        json.dump(errors_out, f, indent=1, sort_keys=True)
        f.write("\n")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    main(sys.argv[1], sys.argv[2])
