"""Generate TRIAGE.md, stubs.md and SUMMARY.md from the sidecars.

Reads sweep.json, stubs.json and errors.json from datadir, and the re-run
results from a repro JSON file; writes the three documents to outdir.
"""
import contextlib
import io
import json
import os
import shlex
from collections import Counter, defaultdict

from .extract import HEX
from .repro import engine_args


def error_signature(scn, e):
    """The scenario's rule for the line, else `kind: text`. FATAL lines take no
    rule: a crash row gets its signature from crash_signature, and a FATAL line
    in any other row reads as the default."""
    t = e["text"]
    if e["kind"] != "FATAL":
        for kind, mode, needle, signature in scn.signature_rules:
            if kind is not None and e["kind"] != kind:
                continue
            hit = needle in t if mode == "contains" else t.startswith(needle)
            if hit:
                return signature.format(last=t.split()[-1])
    return "%s: %s" % (e["kind"], HEX.sub("0x?", t)[:80])


def crash_signature(m, e):
    """The one signature of a crash row, taken from the sweep row: the signal
    that killed the run, else its first recorded FATAL line, else its exit
    code. The extractor keeps only five error lines, so the FATAL line itself
    may be missing from the recorded ones."""
    code = m["exit_code"]
    if isinstance(code, int) and code < 0:
        return "crash: FATAL signal %d" % -code
    for x in e["errors"]:
        if x["kind"] == "FATAL":
            return "crash: FATAL %s" % HEX.sub("0x?", x["text"])[:80]
    return "crash: exit %s" % code


def error_signatures(scn, m, e):
    """The distinct signatures of a row's recorded error lines, in log order.
    In a crash row every FATAL line stands for the one crash signature, which
    comes last when no FATAL line was recorded."""
    crash = m["verdict"] == "crash"
    sigs = [crash_signature(m, e) if crash and x["kind"] == "FATAL" else error_signature(scn, x)
            for x in e["errors"]]
    if crash and not any(x["kind"] == "FATAL" for x in e["errors"]):
        sigs.append(crash_signature(m, e))
    return list(dict.fromkeys(sigs))


def is_loaded(scn, name, maps_loaded):
    if scn.map_match_case_insensitive:
        return name.lower() in [m.lower() for m in maps_loaded]
    return name in maps_loaded


NOT_RECORDED = "not recorded"


def warning_count(m):
    """SCRIPT_WARNING count of a sweep row; None when the sweep that wrote the
    row predates the count, which is unknown and not zero."""
    return m.get("script_warnings")


def warnings_fail(scn, m):
    n = warning_count(m)
    return scn.warnings_fail_clean and n is not None and n > 0


def warnings_total(sweep):
    """Total over all rows, or None unless every row has a count: a partial
    sum would read as a measurement."""
    counts = [warning_count(m) for m in sweep]
    return None if any(c is None for c in counts) else sum(counts)


MATCH_REASON = "match did not start"


def match_unstarted(scn, m):
    """Only a recorded false counts: a sweep that predates the flag, or a
    row without it, is unknown and never makes a map non-clean."""
    return scn.require_match_start and m.get("match_started") is False


def matches_started(sweep):
    return sum(1 for m in sweep if m.get("match_started") is True)


def show_count(n):
    return NOT_RECORDED if n is None else str(n)


def _load(datadir, name):
    with open(os.path.join(datadir, name)) as f:
        return json.load(f)


@contextlib.contextmanager
def _document(outdir, name):
    """A buffer that is written to outdir/name as one document ending in
    exactly one newline, whatever the pieces written to it ended with."""
    buf = io.StringIO()
    yield buf
    with open(os.path.join(outdir, name), "w") as f:
        f.write(buf.getvalue().rstrip() + "\n")


def build(scn, datadir, outdir, repro_path):
    sweep = _load(datadir, "sweep.json")["maps"]
    stubs = _load(datadir, "stubs.json")
    errs = _load(datadir, "errors.json")
    with open(repro_path) as f:
        repro = json.load(f)
    owner_file = scn.stub_owner_file
    b = scn.bullet

    rows = []
    for m in sweep:
        name = m["map"]
        gp = [s for s in stubs["maps"][name] if s["class"] != "non-gameplay"]
        e = errs[name]
        reasons = []
        if m["verdict"] not in ("pass", "crash"):
            reasons.append("verdict " + m["verdict"])
        if not is_loaded(scn, name, m["maps_loaded"]) or m["fps_avg"] is None:
            reasons.append("idle" if m["verdict"] == "pass" else "no render")
        reasons += error_signatures(scn, m, e)
        if warnings_fail(scn, m):
            reasons.append("warnings")
        if match_unstarted(scn, m):
            reasons.append(MATCH_REASON)
        if e["fatal_after_summary"]:
            reasons.append("FATAL after SUMMARY")
        reasons += ["stub " + s["text"] for s in gp]
        rows.append((m, e, gp, sorted(set(reasons))))
    nonclean = [r for r in rows if r[3]]

    with _document(outdir, "TRIAGE.md") as f:
        f.write("# %s sweep triage\n\n" % scn.label)
        f.write(scn.triage_intro.format(nonclean=len(nonclean), total=len(rows)))
        f.write(scn.triage_evidence)
        for i, (m, e, gp, reasons) in enumerate(nonclean):
            name = m["map"]
            if i:
                f.write("\n")
            f.write("## %s\n\n" % name)
            f.write(b + scn.verdict_template.format(verdict=m["verdict"], exit_code=m["exit_code"]) + "\n")
            f.write("%smaps_loaded: %s\n" % (b, json.dumps(m["maps_loaded"])))
            if e["errors"]:
                f.write("%sfailing lines (first %d distinct, log order):\n" % (b, len(e["errors"])))
                for x in e["errors"]:
                    f.write("  %s`%s %s`\n" % (b, x["kind"], x["text"]))
                if e["fatal_after_summary"]:
                    f.write("%sfatal_after_summary: true\n" % b)
            else:
                f.write("%sfailing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)\n" % b)
            f.write("%swarnings: %s\n" % (b, show_count(warning_count(m))))
            if scn.require_match_start:
                f.write("%smatch_started: %s\n" % (b, json.dumps(m.get("match_started"))))
            f.write("%sgameplay stubs: %s\n" % (b, ", ".join("`%s`" % s["text"] for s in gp) or "none"))
            argv = " ".join(shlex.quote(a) for a in engine_args(scn, scn.exe, name))
            f.write("%srepro: `%s \"%s\"`\n" % (b, argv, scn.game_placeholder))
            sigs = error_signatures(scn, m, e)
            if warnings_fail(scn, m):
                sigs.append("warnings")
            if match_unstarted(scn, m):
                sigs.append(MATCH_REASON)
            owners, cites = [], []
            for s in dict.fromkeys(sigs):
                o, c = scn.error_owner.get(s, ("unattributed (%s)" % s, "none applies"))
                owners.append(o)
                cites.append(c)
            if gp:
                files = sorted({owner_file.get(s["owner"], "unknown owner") for s in gp})
                owners.append("unimplemented natives, %s: the gameplay stubs above are LogUnimplemented bodies" % ", ".join(files))
                cites.append(scn.status_natives)
            f.write("%sowner: %s\n" % (b, "; ".join(owners)))
            f.write("%s%s: %s\n" % (b, scn.status_label, "; ".join(dict.fromkeys(cites))))
            r = repro.get(name)
            if r:
                f.write(b + scn.reproduced_template.format(
                    reproduced=r["reproduced"], exit_code=r["exit_code"],
                    run1=HEX.sub("0x?", str(r["first_line_run1"]))[:200],
                    run2=HEX.sub("0x?", str(r["first_line_run2"]))[:200]) + "\n")
            else:
                f.write("%sreproduced: not-run\n" % b)

    count = Counter()
    for lst in stubs["maps"].values():
        for s in lst:
            count[s["text"]] += 1
    cls = {}
    for lst in stubs["maps"].values():
        for s in lst:
            cls[s["text"]] = s
    with _document(outdir, "stubs.md") as f:
        f.write("# Distinct STUB messages, %s sweep\n\n" % scn.label)
        f.write("Distinct stub lines in the logs: %d. Rows below: %d.\n\n" % (len(count), len(count)))
        f.write("| Stub | Maps | Class | Reason |\n| :--- | ---: | :--- | :--- |\n")
        for text, n in sorted(count.items(), key=lambda kv: (-kv[1], kv[0])):
            s = cls[text]
            f.write("| `%s` | %d | %s | %s |\n" % (text, n, s["class"], stubs["owners"][s["owner"]]["reason"]))
        f.write(scn.stubs_footer)

    verdicts = Counter(m["verdict"] for m, *_ in rows)
    clean = len(rows) - len(nonclean)
    sig_maps = defaultdict(set)
    for m, e, gp, reasons in nonclean:
        for r in reasons:
            sig_maps[r].add(m["map"])
    file_maps = defaultdict(set)
    for m, e, gp, reasons in nonclean:
        for s in gp:
            file_maps[owner_file.get(s["owner"], "unknown")].add(m["map"])
    reasons_of = {m["map"]: set(r) for m, e, gp, r in nonclean}
    stubfile_of = {}
    for m, e, gp, r in nonclean:
        stubfile_of[m["map"]] = {"stub " + s["text"]: owner_file.get(s["owner"], "unknown") for s in gp}

    warning_lines, warning_maps = Counter(), Counter()
    for m in sweep:
        for site, n in errs[m["map"]].get("warning_sites", {}).items():
            warning_lines[site] += n
            warning_maps[site] += 1

    with _document(outdir, "SUMMARY.md") as f:
        f.write("# %s sweep summary\n\n" % scn.label)
        f.write(scn.counts_template.format(
            maps=len(rows), clean=clean, fail=verdicts["fail"], crash=verdicts["crash"],
            timeout=verdicts["timeout"],
            stub_only=sum(1 for m, e, gp, r in nonclean if m["verdict"] == "pass"),
            warnings=show_count(warnings_total(sweep)),
            matches_started=matches_started(sweep)) + "\n\n")
        f.write("Check: the verdict counts are recomputed from sweep.json by the `check`\n"
                "command of the triage module, which exits 0 only if they equal the line\n"
                "above.\n\n")
        f.write("## Warning sites\n\n| Site | Lines | Maps |\n| :--- | ---: | ---: |\n")
        for site, n in sorted(warning_lines.items(), key=lambda kv: (-kv[1], kv[0])):
            f.write("| %s | %d | %d |\n" % (site.replace("|", "\\|"), n, warning_maps[site]))
        if not warning_lines:
            f.write("| none | | |\n")
        f.write("\n## Failure signatures\n\n| Signature | Maps | Affected |\n| :--- | ---: | :--- |\n")
        for s, ms in sorted(sig_maps.items(), key=lambda kv: (-len(kv[1]), kv[0])):
            if len(ms) <= 5:
                aff = ", ".join(sorted(ms))
            else:
                aff = "%d maps (see TRIAGE.md)" % len(ms)
            f.write("| %s | %d | %s |\n" % (s, len(ms), aff))
        if not sig_maps:
            f.write("| none | | |\n")
        f.write("\n## Ranked fix candidates\n\n")
        f.write("Fully unblocks means every non-clean reason of the map is covered by the\n"
                "candidate. Partial means the map loses one reason but keeps others.\n\n")
        cands = []
        for fl, ms in file_maps.items():
            covered = [mp for mp in ms if all(rs in stubfile_of[mp] and stubfile_of[mp][rs] == fl for rs in reasons_of[mp])]
            cands.append(("implement every gameplay stub in " + fl, len(covered), len(ms) - len(covered)))
        for s, ms in sig_maps.items():
            if not s.startswith("stub "):
                covered = [mp for mp in ms if reasons_of[mp] == {s}]
                cands.append(("fix signature: " + s, len(covered), len(ms) - len(covered)))
        cands.sort(key=lambda c: (-c[1], -c[2], c[0]))
        f.write("| Rank | Candidate | Fully unblocks | Partial |\n| ---: | :--- | ---: | ---: |\n")
        for i, (c, full, part) in enumerate(cands, 1):
            f.write("| %d | %s | %d | %d |\n" % (i, c, full, part))
        if not cands:
            f.write("| none | | | |\n")
        f.write(scn.summary_footer)
