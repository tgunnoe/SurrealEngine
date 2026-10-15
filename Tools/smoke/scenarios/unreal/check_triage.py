#!/usr/bin/env python3
"""Check that TRIAGE.md covers exactly the non-clean maps of sweep.json.

usage: python3 -I check_triage.py   (reads the files beside this script)

A map is clean when: verdict is pass, the requested map is in maps_loaded,
fps_avg is not null, there are no script errors, missing natives,
exceptions or FATAL lines (including after SUMMARY), and no STUB whose
"gameplay" flag is true or null (null = owner not in either class set,
treated as non-clean).
"""
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPRO_VALUES = ("yes", "no", "flaky", "not-run")
# The first N non-clean maps, in sweep order, must have been re-run.
MUST_REPRODUCE = 5


def load(name):
    with open(os.path.join(HERE, name)) as f:
        return json.load(f)


def non_clean_reasons(row, stubs, errors):
    why = []
    if row["verdict"] != "pass":
        why.append("verdict " + row["verdict"])
    if row["map"].lower() not in [m.lower() for m in row["maps_loaded"]]:
        why.append("idle: requested map not in maps_loaded")
    if row["fps_avg"] is None:
        why.append("idle: fps_avg null")
    for key in ("script_errors", "missing_natives", "exceptions"):
        if row[key]:
            why.append("%s=%d" % (key, row[key]))
    err = errors[row["map"]]
    if any(e["kind"] == "FATAL" for e in err["errors"]) or err["fatal_after_summary"]:
        why.append("FATAL")
    for s in stubs[row["map"]]:
        if s["gameplay"] is not False:
            why.append("gameplay or unclassified stub " + s["text"])
    return why


def sections(md):
    out = {}
    name = None
    for line in md.splitlines():
        m = re.match(r"## (\S+)\s*$", line)
        if m:
            name = m.group(1)
            out[name] = []
        elif name is not None:
            out[name].append(line)
    return {k: "\n".join(v) for k, v in out.items()}


def main():
    sweep, stubs, errors = load("sweep.json"), load("stubs.json"), load("errors.json")
    problems = []
    rows = sweep["maps"]
    noncl = [r for r in rows if non_clean_reasons(r, stubs, errors)]
    with open(os.path.join(HERE, "TRIAGE.md")) as f:
        secs = sections(f.read())
    names = [r["map"] for r in noncl]
    if set(secs) != set(names):
        problems.append("section set differs: missing %s extra %s" %
                        (sorted(set(names) - set(secs)), sorted(set(secs) - set(names))))
    for i, r in enumerate(noncl):
        body = secs.get(r["map"])
        if body is None:
            continue
        m = r["map"]
        rep = re.search(r"^reproduced: (\S+)", body, re.M)
        if not rep or rep.group(1) not in REPRO_VALUES:
            problems.append(m + ": no valid reproduced: line")
        elif i < MUST_REPRODUCE and rep.group(1) == "not-run":
            problems.append(m + ": among the first %d, must be re-run" % MUST_REPRODUCE)
        elif rep.group(1) != "not-run" and not re.search(r"^reproduced: \S+ \(exit -?\d+\)", body, re.M):
            problems.append(m + ": reproduced: line lacks the second run's exit code")
        ml = re.search(r"^maps_loaded: (.+)$", body, re.M)
        if not ml or json.dumps(r["maps_loaded"]) not in ml.group(1):
            problems.append(m + ": maps_loaded line missing or differs from sweep.json")
        if not re.search(r"^repro: .*--url=%s .*\S" % re.escape(m), body, re.M):
            problems.append(m + ": no repro command with --url=" + m)
        if not re.search(r"^owner: \S.*", body, re.M):
            problems.append(m + ": no owner line")
        if not re.search(r"^verdict: %s, exit %d" % (r["verdict"], r["exit_code"]), body, re.M):
            problems.append(m + ": verdict/exit line missing or differs from sweep.json")
        for e in errors[m]["errors"]:
            if e["text"] not in body:
                problems.append("%s: error line absent from section: %s" % (m, e["text"][:60]))
        for s in stubs[m]:
            if s["gameplay"] is not False and s["text"] not in body:
                problems.append("%s: gameplay stub absent from section: %s" % (m, s["text"]))
    for p in problems:
        print("FAIL " + p)
    print("non-clean: %d of %d" % (len(noncl), len(rows)))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
