"""Extract per-map STUB and error sidecars from a directory of smoke logs.

Writes <outdir>/stubs.json and <outdir>/errors.json. See Tools/smoke/README.md
for the log grammar.
"""
import json
import os
import re

ERROR_KINDS = ("SCRIPT_ERROR", "MISSING_NATIVE", "EXCEPTION", "FATAL")
HEX = re.compile(r"0x[0-9a-fA-F]+")

# A native backtrace frame: "at <binary>(+0xOFF) [0xADDR]". Frames carry
# build paths and per-run addresses, so the message is cut at the first one.
NATIVE_FRAME = re.compile(r"\s+at \S*\(\S*\+0x[0-9a-fA-F]+\) \[0x[0-9a-fA-F]+\]")


def classify(scn, owner):
    """Return (class, reason) for a stub owner; class is gameplay,
    non-gameplay or unclassified. A class is gameplay when its chain meets
    one of the scenario's gameplay roots."""
    chain = scn.hierarchy.get(owner)
    if chain is None:
        return "unclassified", "owner not in HIERARCHY; extend the table"
    hit = [c for c in chain if c in scn.gameplay_roots]
    if hit:
        return "gameplay", "%s descends from %s" % (owner, hit[0])
    return "non-gameplay", "%s descends only from %s" % (owner, " > ".join(chain))


def owner_of(scn, text):
    """Owner class of a stub text: the prefix before the first dot, else the
    scenario's bare-owner entry for the call name, else "" (unclassified)."""
    if "." in text:
        return text.split(".", 1)[0]
    name = text.split(" ", 1)[0].split("(", 1)[0]
    return scn.bare_owners.get(name, "")


def strip_native_stack(text):
    m = NATIVE_FRAME.search(text)
    if not m:
        return text
    text = text[:m.start()].rstrip()
    if text.endswith("Native call stack:"):
        text = text[:-len("Native call stack:")].rstrip()
    return text


def parse(path):
    """Return (distinct stub texts, first five distinct errors, whether a
    FATAL followed the SUMMARY line)."""
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


def extract(scn, logdir, outdir):
    stubs_out, errors_out, owners = {}, {}, {}
    for name in sorted(os.listdir(logdir)):
        if not name.endswith(".log"):
            continue
        m = name[:-4]
        stubs, errors, fas = parse(os.path.join(logdir, name))
        rows = []
        for s in stubs:
            owner = owner_of(scn, s)
            cls, reason = classify(scn, owner)
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
