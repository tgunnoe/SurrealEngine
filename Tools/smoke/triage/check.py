"""Check TRIAGE.md, stubs.md and SUMMARY.md against the sidecars.

Which checks run is set by the Scenario's option fields; every problem is
returned as a message naming the map or document it concerns.
"""
import json
import os
import re
import string
from collections import Counter

from .build import is_loaded

# With require_first_five, the first N non-clean maps in sweep order must
# have been re-run (not "not-run").
FIRST_N = 5
REPRO_VALUES = "yes|no|flaky|not-run"


def _load(d, name):
    with open(os.path.join(d, name)) as f:
        return json.load(f)


def _text(d, name):
    with open(os.path.join(d, name)) as f:
        return f.read()


def template_regex(template, **fragments):
    """Regex for a str.format template: literal text escaped, each field
    replaced by the given regex fragment."""
    out = []
    for literal, field, _, _ in string.Formatter().parse(template):
        out.append(re.escape(literal))
        if field is not None:
            out.append(fragments[field])
    return "".join(out)


def check(scn, datadir, docdir):
    """Return (sections, nonclean, problems)."""
    bad = []
    sweep = _load(datadir, "sweep.json")["maps"]
    stubs = _load(datadir, "stubs.json")
    errs = _load(datadir, "errors.json")
    b = re.escape(scn.bullet)

    if scn.row_count is not None and len(sweep) != scn.row_count:
        bad.append("sweep.json: expected %d rows, found %d" % (scn.row_count, len(sweep)))
    if scn.screenshots_null and any(m["screenshot"] is not None for m in sweep):
        bad.append("sweep.json: screenshot not null in every row")

    nonclean = []
    for m in sweep:
        n = m["map"]
        gp = [s for s in stubs["maps"][n] if s["class"] != "non-gameplay"]
        e = errs[n]
        if (m["verdict"] != "pass" or not is_loaded(scn, n, m["maps_loaded"]) or m["fps_avg"] is None
                or e["errors"] or e["fatal_after_summary"] or gp):
            nonclean.append(n)
    nonclean_set = set(nonclean)
    row_of = {m["map"]: m for m in sweep}

    triage = _text(docdir, "TRIAGE.md")
    parts = re.split(r"^## ", triage, flags=re.M)[1:]
    sections = {p.split("\n", 1)[0].strip(): p for p in parts}
    if len(sections) != len(parts):
        bad.append("TRIAGE.md: duplicate section")
    if set(sections) != nonclean_set:
        bad.append("section set differs from non-clean set: extra=%s missing=%s"
                   % (sorted(set(sections) - nonclean_set), sorted(nonclean_set - set(sections))))

    repro_rx = r"^%srepro: `%s --autostart --exit-after=%d --smoke-log=\S+ --url=%%s " % (
        b, re.escape(scn.exe), scn.exit_after_seconds)
    for n, body in sections.items():
        for label, pat in (("reproduced line", r"^%sreproduced: (%s)" % (b, REPRO_VALUES)),
                           ("maps_loaded line", r"^%smaps_loaded: " % b),
                           ("repro command", repro_rx % re.escape(n)),
                           ("owner line", r"^%sowner: \S" % b)):
            if not re.search(pat, body, flags=re.M):
                bad.append("%s: missing %s" % (n, label))
        row = row_of.get(n)
        if scn.errors_in_section:
            for x in errs.get(n, {"errors": []})["errors"]:
                if "`%s %s`" % (x["kind"], x["text"]) not in body:
                    bad.append("%s: error line not verbatim: %s" % (n, x["text"][:60]))
        if scn.gameplay_stubs_in_section:
            for s in stubs["maps"].get(n, []):
                if s["class"] != "non-gameplay" and s["text"] not in body:
                    bad.append("%s: gameplay stub absent from section: %s" % (n, s["text"]))
        if scn.reproduced_exit_code:
            rx = template_regex(scn.reproduced_template, reproduced="(?:yes|no|flaky)",
                                exit_code=r"-?\d+", run1=".*", run2=".*")
            if not re.search(r"^%s%s" % (b, rx), body, flags=re.M) and not re.search(
                    r"^%sreproduced: not-run" % b, body, flags=re.M):
                bad.append("%s: reproduced line lacks the second run's exit code" % n)
        if row is None:
            continue
        if scn.maps_loaded_equal:
            ml = re.search(r"^%smaps_loaded: (.+)$" % b, body, flags=re.M)
            if not ml or json.dumps(row["maps_loaded"]) not in ml.group(1):
                bad.append("%s: maps_loaded line missing or differs from sweep.json" % n)
        if scn.verdict_line_equal:
            rx = template_regex(scn.verdict_template, verdict=re.escape(str(row["verdict"])),
                                exit_code=re.escape(str(row["exit_code"])))
            if not re.search(r"^%s%s$" % (b, rx), body, flags=re.M):
                bad.append("%s: verdict line missing or differs from sweep.json" % n)

    if scn.require_first_five:
        for n in nonclean[:FIRST_N]:
            body = sections.get(n)
            if body is not None and re.search(r"^%sreproduced: not-run" % b, body, flags=re.M):
                bad.append("%s: among the first %d non-clean maps, must be re-run" % (n, FIRST_N))

    if scn.stubs_md_count:
        distinct = {s["text"] for lst in stubs["maps"].values() for s in lst}
        md = _text(docdir, "stubs.md")
        rows = [l for l in md.splitlines() if l.startswith("| `")]
        m = re.search(r"Distinct stub lines in the logs: (\d+)\. Rows below: (\d+)\.", md)
        if not m or int(m.group(1)) != len(distinct) or int(m.group(2)) != len(rows) or len(rows) != len(distinct):
            bad.append("stubs.md: counts do not match %d distinct stubs" % len(distinct))

    if scn.summary_counts_line:
        v = Counter(m["verdict"] for m in sweep)
        stub_only = sum(1 for n in nonclean_set if row_of[n]["verdict"] == "pass")
        line = scn.counts_template.format(
            maps=len(sweep), clean=len(sweep) - len(nonclean_set), fail=v["fail"],
            crash=v["crash"], timeout=v["timeout"], stub_only=stub_only)
        if line not in _text(docdir, "SUMMARY.md"):
            bad.append("SUMMARY.md: counts line differs from: " + line)

    return len(sections), len(nonclean_set), bad


def format_result(sections, nonclean, problems):
    lines = ["FAIL " + p for p in problems]
    lines.append("sections=%d nonclean=%d problems=%d" % (sections, nonclean, len(problems)))
    return "\n".join(lines)
