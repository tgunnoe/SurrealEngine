"""Generate TRIAGE.md, stubs.md and SUMMARY.md from the sidecars.

Reads sweep.json, stubs.json and errors.json from datadir, and the re-run
results from a repro JSON file; writes the three documents to outdir.
"""
import json
import os
from collections import Counter, defaultdict

from .extract import HEX


def error_signature(scn, e):
    t = e["text"]
    for kind, mode, needle, signature in scn.signature_rules:
        if kind is not None and e["kind"] != kind:
            continue
        hit = needle in t if mode == "contains" else t.startswith(needle)
        if hit:
            return signature.format(last=t.split()[-1])
    return "%s: %s" % (e["kind"], HEX.sub("0x?", t)[:80])


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


def show_count(n):
    return NOT_RECORDED if n is None else str(n)


def _load(datadir, name):
    with open(os.path.join(datadir, name)) as f:
        return json.load(f)


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
        if m["verdict"] != "pass":
            reasons.append("verdict " + m["verdict"])
        if not is_loaded(scn, name, m["maps_loaded"]) or m["fps_avg"] is None:
            reasons.append("idle" if m["verdict"] == "pass" else "no render")
        reasons += [error_signature(scn, x) for x in e["errors"]]
        if warnings_fail(scn, m):
            reasons.append("warnings")
        if e["fatal_after_summary"]:
            reasons.append("FATAL after SUMMARY")
        reasons += ["stub " + s["text"] for s in gp]
        rows.append((m, e, gp, sorted(set(reasons))))
    nonclean = [r for r in rows if r[3]]

    with open(os.path.join(outdir, "TRIAGE.md"), "w") as f:
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
            f.write("%sgameplay stubs: %s\n" % (b, ", ".join("`%s`" % s["text"] for s in gp) or "none"))
            argv = [scn.exe, "--autostart", "--exit-after=%d" % scn.exit_after_seconds,
                    "--smoke-log=%s/%s.log" % (scn.repro_logdir, name), "--url=%s" % name,
                    *scn.url_options]
            f.write("%srepro: `%s \"%s\"`\n" % (b, " ".join(argv), scn.game_placeholder))
            sigs = [error_signature(scn, x) for x in e["errors"]]
            if warnings_fail(scn, m):
                sigs.append("warnings")
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
    with open(os.path.join(outdir, "stubs.md"), "w") as f:
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

    with open(os.path.join(outdir, "SUMMARY.md"), "w") as f:
        f.write("# %s sweep summary\n\n" % scn.label)
        f.write(scn.counts_template.format(
            maps=len(rows), clean=clean, fail=verdicts["fail"], crash=verdicts["crash"],
            timeout=verdicts["timeout"],
            stub_only=sum(1 for m, e, gp, r in nonclean if m["verdict"] == "pass"),
            warnings=show_count(warnings_total(sweep)), matches_started=-1) + "\n\n")
        f.write("Check: the verdict counts are recomputed from sweep.json by check_triage.py,\n"
                "which exits 0 only if they equal the line above.\n\n")
        f.write("## Failure signatures\n\n| Signature | Maps | Affected |\n| :--- | ---: | :--- |\n")
        for s, ms in sorted(sig_maps.items(), key=lambda kv: (-len(kv[1]), kv[0])):
            if len(ms) <= 5:
                aff = ", ".join(sorted(ms))
            else:
                aff = "%d maps (see TRIAGE.md)" % len(ms)
            f.write("| %s | %d | %s |\n" % (s, len(ms), aff))
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
        f.write(scn.summary_footer)
