"""Check that TRIAGE.md has exactly one section per non-clean map.

usage: python3 -I check_triage.py   (reads the files beside this script)

A row is clean only if verdict is pass, the requested map is in
maps_loaded, fps_avg is not null, no error line and no FATAL after SUMMARY
was recorded, and no STUB has a gameplay or unresolved owner.
"""
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REQUIRED = ("maps_loaded:", "repro:", "owner:", "reproduced:")


def load(name):
    with open(os.path.join(HERE, name)) as f:
        return json.load(f)


def non_clean_reasons(row, stubs, errors):
    why = []
    m = row["map"]
    if row["verdict"] != "pass":
        why.append("verdict " + row["verdict"])
    if m not in row["maps_loaded"] or row["fps_avg"] is None:
        why.append("idle")
    if (errors[m]["first"] or errors[m]["fatal_after_summary"]
            or row["script_errors"] or row["missing_natives"] or row["exceptions"]):
        why.append("error lines")
    if any(s["gameplay"] is not False for s in stubs[m]):
        why.append("gameplay stub")
    return why


def sections(text):
    parts = re.split(r"^## (.+)$", text, flags=re.M)
    return {parts[i].strip(): parts[i + 1] for i in range(1, len(parts), 2)}


def main():
    sweep, stubs, errors = load("sweep.json"), load("stubs.json"), load("errors.json")
    with open(os.path.join(HERE, "TRIAGE.md")) as f:
        secs = sections(f.read())
    bad = []
    rows = sweep["maps"]
    if len(rows) != 83:
        bad.append("sweep.json has %d rows, want 83" % len(rows))
    want = {r["map"] for r in rows if non_clean_reasons(r, stubs, errors)}
    if want != set(secs):
        bad.append("missing sections: %s" % sorted(want - set(secs)))
        bad.append("extra sections: %s" % sorted(set(secs) - want))
    for name, body in secs.items():
        for key in REQUIRED:
            if key not in body:
                bad.append("%s: no %s line" % (name, key))
        if not re.search(r"^reproduced: (yes|no|flaky|not-run)", body, re.M):
            bad.append("%s: reproduced line malformed" % name)
        if "--url=" + name not in body:
            bad.append("%s: repro command does not name the map" % name)
    for b in bad:
        print(b)
    print("non-clean: %d, sections: %d" % (len(want), len(secs)))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
