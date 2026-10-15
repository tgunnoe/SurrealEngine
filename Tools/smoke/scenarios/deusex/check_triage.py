"""Check TRIAGE.md, stubs.md and SUMMARY.md against the committed data.

usage: python3 -I check_triage.py   (any directory; reads files beside it)
Exit 0 only when every check holds.
"""
import json
import os
import re
import sys
from collections import Counter

D = os.path.dirname(os.path.abspath(__file__))


def load(n):
    with open(os.path.join(D, n)) as f:
        return json.load(f)


def text(n):
    with open(os.path.join(D, n)) as f:
        return f.read()


def main():
    bad = []
    sweep = load("sweep.json")["maps"]
    stubs = load("stubs.json")
    errs = load("errors.json")

    if len(sweep) != 88 or any(m["screenshot"] is not None for m in sweep):
        bad.append("sweep.json: need 88 rows with null screenshot")

    nonclean = set()
    for m in sweep:
        n = m["map"]
        gp = [s for s in stubs["maps"][n] if s["class"] != "non-gameplay"]
        e = errs[n]
        if (m["verdict"] != "pass" or n not in m["maps_loaded"] or m["fps_avg"] is None
                or e["errors"] or e["fatal_after_summary"] or gp):
            nonclean.add(n)

    triage = text("TRIAGE.md")
    parts = re.split(r"^## ", triage, flags=re.M)[1:]
    sections = {p.split("\n", 1)[0].strip(): p for p in parts}
    if len(sections) != len(parts):
        bad.append("TRIAGE.md: duplicate section")
    if set(sections) != nonclean:
        bad.append("section set differs from non-clean set: extra=%s missing=%s"
                   % (sorted(set(sections) - nonclean), sorted(nonclean - set(sections))))

    for n, body in sections.items():
        for label, pat in (("reproduced line", r"^- reproduced: (yes|no|flaky|not-run)"),
                           ("maps_loaded line", r"^- maps_loaded: "),
                           ("repro command", r"^- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=\S+ --url=%s " % re.escape(n)),
                           ("owner line", r"^- owner: \S")):
            if not re.search(pat, body, flags=re.M):
                bad.append("%s: missing %s" % (n, label))
        for x in errs.get(n, {"errors": []})["errors"]:
            if "`%s %s`" % (x["kind"], x["text"]) not in body:
                bad.append("%s: error line not verbatim: %s" % (n, x["text"][:60]))

    # stubs.md: distinct stub count vs rows
    distinct = {s["text"] for lst in stubs["maps"].values() for s in lst}
    md = text("stubs.md")
    rows = [l for l in md.splitlines() if l.startswith("| `")]
    m = re.search(r"Distinct stub lines in the logs: (\d+)\. Rows below: (\d+)\.", md)
    if not m or int(m.group(1)) != len(distinct) or int(m.group(2)) != len(rows) or len(rows) != len(distinct):
        bad.append("stubs.md: counts do not match %d distinct stubs" % len(distinct))

    # SUMMARY.md counts
    v = Counter(m["verdict"] for m in sweep)
    stub_only = sum(1 for n in nonclean if next(x for x in sweep if x["map"] == n)["verdict"] == "pass")
    line = ("Maps: %d. clean: %d. fail: %d. crash: %d. timeout: %d. pass-verdict but non-clean (stub-only): %d."
            % (len(sweep), len(sweep) - len(nonclean), v["fail"], v["crash"], v["timeout"], stub_only))
    if line not in text("SUMMARY.md"):
        bad.append("SUMMARY.md: counts line differs from: " + line)

    for b in bad:
        print("FAIL", b)
    print("sections=%d nonclean=%d problems=%d" % (len(sections), len(nonclean), len(bad)))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
