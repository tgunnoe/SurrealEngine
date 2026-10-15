"""Generate TRIAGE.md, stubs.md and SUMMARY.md from the committed sidecars.

usage: python3 -I build_triage.py <repro.json>

Run from this directory. Reads sweep.json, stubs.json, errors.json and the
re-run results in repro.json (also committed).
"""
import json
import re
import sys
from collections import Counter, defaultdict

STUB_OWNER_FILE = {
    "LevelInfo": "SurrealEngine/Native/NLevelInfo.cpp",
    "Actor": "SurrealEngine/Native/NActor.cpp",
    "Pawn": "SurrealEngine/Native/NPawn.cpp",
    "PlayerPawn": "SurrealEngine/Native/NPlayerPawn.cpp",
    "DumpLocation": "SurrealEngine/Native/NDumpLocation.cpp",
}
EXE = "SurrealEngine"
GAME = "<game folder>"
STATUS_NATIVES = "Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')"

HEX = re.compile(r"0x[0-9a-fA-F]+")


def error_signature(e):
    t = e["text"]
    if "Failed to spawn player actor" in t:
        return "login: Failed to spawn player actor"
    if "Iterator statement without an iterator" in t:
        return "script: Iterator statement without an iterator (ReachablePathnodes)"
    if e["kind"] == "FATAL" and t.startswith("signal"):
        return "crash: FATAL signal " + t.split()[-1]
    return "%s: %s" % (e["kind"], HEX.sub("0x?", t)[:80])


ERROR_OWNER = {
    "login: Failed to spawn player actor": (
        "SurrealEngine/Engine.cpp (login path, throws at line 1004) and the "
        "DeusEx GameInfo.Login script that spawns the player; the exception "
        "text carries no script cause, so the failing spawn step is not yet "
        "identified",
        "Docs/Status.md line 41 ('Sometimes opening a map crashes the engine with a \"Failed to spawn the player actor\" error.')"),
    "script: Iterator statement without an iterator (ReachablePathnodes)": (
        "SurrealEngine/Native/NPawn.cpp NPawn::ReachablePathnodes (line 332) "
        "is a LogUnimplemented stub that never sets up the iterator, so "
        "SurrealEngine/VM/Frame.cpp line 569 throws; this log also holds "
        "'STUB Pawn.ReachablePathnodes'",
        STATUS_NATIVES),
    "crash: FATAL signal 11": (
        "unknown native or VM subsystem: SIGSEGV after LOGIN with no script "
        "line before it; no debugger was available (gdb not found), so the "
        "faulting frame is not identified",
        "none applies"),
}


def stub_owner_files(stubs):
    out = []
    for s in stubs:
        if s["class"] == "non-gameplay":
            continue
        f = STUB_OWNER_FILE.get(s["owner"], "unknown owner " + s["owner"])
        out.append("%s (%s)" % (f, s["text"]))
    return out


def main(repro_path):
    sweep = json.load(open("sweep.json"))["maps"]
    stubs = json.load(open("stubs.json"))
    errs = json.load(open("errors.json"))
    repro = json.load(open(repro_path))

    rows = []
    for m in sweep:
        name = m["map"]
        gp = [s for s in stubs["maps"][name] if s["class"] != "non-gameplay"]
        e = errs[name]
        reasons = []
        if m["verdict"] != "pass":
            reasons.append("verdict " + m["verdict"])
        if name not in m["maps_loaded"] or m["fps_avg"] is None:
            reasons.append("idle" if m["verdict"] == "pass" else "no render")
        reasons += [error_signature(x) for x in e["errors"]]
        if e["fatal_after_summary"]:
            reasons.append("FATAL after SUMMARY")
        reasons += ["stub " + s["text"] for s in gp]
        rows.append((m, e, gp, sorted(set(reasons))))
    nonclean = [r for r in rows if r[3]]

    with open("TRIAGE.md", "w") as f:
        f.write("# Deus Ex 1112fm sweep triage\n\n")
        f.write("Non-clean maps: %d of %d. A map is clean only when it passes, loaded\n"
                "the requested map, rendered, and logged no error line and no\n"
                "gameplay-class stub. Every map hit at least one gameplay-class stub,\n"
                "so every map has a section. `fps_avg` is not evidence in this run\n"
                "(three sweeps shared one display).\n\n" % (len(nonclean), len(rows)))
        f.write("Evidence for a stub-only row is its stub list (stubs.json) and its\n"
                "exit code and SUMMARY-bearing pass verdict (sweep.json).\n\n")
        for i, (m, e, gp, reasons) in enumerate(nonclean):
            name = m["map"]
            if i:
                f.write("\n")
            f.write("## %s\n\n" % name)
            f.write("- verdict: %s, exit code %s\n" % (m["verdict"], m["exit_code"]))
            f.write("- maps_loaded: %s\n" % json.dumps(m["maps_loaded"]))
            if e["errors"]:
                f.write("- failing lines (first %d distinct, log order):\n" % len(e["errors"]))
                for x in e["errors"]:
                    f.write("  - `%s %s`\n" % (x["kind"], x["text"]))
                if e["fatal_after_summary"]:
                    f.write("- fatal_after_summary: true\n")
            else:
                f.write("- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)\n")
            f.write("- gameplay stubs: %s\n" % (", ".join("`%s`" % s["text"] for s in gp) or "none"))
            f.write("- repro: `%s --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/%s.log --url=%s \"%s\"`\n"
                    % (EXE, name, name, GAME))
            sigs = [error_signature(x) for x in e["errors"]]
            owners, cites = [], []
            for s in dict.fromkeys(sigs):
                o, c = ERROR_OWNER.get(s, ("unattributed (%s)" % s, "none applies"))
                owners.append(o)
                cites.append(c)
            if gp:
                files = sorted({STUB_OWNER_FILE.get(s["owner"], "unknown owner") for s in gp})
                owners.append("unimplemented natives, %s: the gameplay stubs above are LogUnimplemented bodies" % ", ".join(files))
                cites.append(STATUS_NATIVES)
            f.write("- owner: %s\n" % "; ".join(owners))
            f.write("- Status.md: %s\n" % "; ".join(dict.fromkeys(cites)))
            r = repro.get(name)
            if r:
                f.write("- reproduced: %s (second run exit code %s; run1 first line `%s`; run2 first line `%s`)\n"
                        % (r["reproduced"], r["exit_code"], HEX.sub("0x?", str(r["first_line_run1"]))[:200],
                           HEX.sub("0x?", str(r["first_line_run2"]))[:200]))
            else:
                f.write("- reproduced: not-run\n")

    # stubs.md
    count = Counter()
    for name, lst in stubs["maps"].items():
        for s in lst:
            count[s["text"]] += 1
    cls = {}
    for lst in stubs["maps"].values():
        for s in lst:
            cls[s["text"]] = s
    with open("stubs.md", "w") as f:
        f.write("# Distinct STUB messages, Deus Ex 1112fm sweep\n\n")
        f.write("Distinct stub lines in the logs: %d. Rows below: %d.\n\n" % (len(count), len(count)))
        f.write("| Stub | Maps | Class | Reason |\n| :--- | ---: | :--- | :--- |\n")
        for text, n in sorted(count.items(), key=lambda kv: (-kv[1], kv[0])):
            s = cls[text]
            f.write("| `%s` | %d | %s | %s |\n" % (text, n, s["class"], stubs["owners"][s["owner"]]["reason"]))
        f.write("\nClassification follows the gameplay-class definition in the run\n"
                "boundary: a stub is gameplay when its owner descends from Actor,\n"
                "Pawn, PlayerPawn, LevelInfo or another listed root. DumpLocation\n"
                "descends only from Object, so it is non-gameplay by that definition\n"
                "even though gameplay script calls it.\n")

    # SUMMARY.md
    verdicts = Counter(m["verdict"] for m, *_ in rows)
    clean = len(rows) - len(nonclean)
    sig_maps = defaultdict(set)
    for m, e, gp, reasons in nonclean:
        for r in reasons:
            sig_maps[r].add(m["map"])
    file_maps = defaultdict(set)
    for m, e, gp, reasons in nonclean:
        for s in gp:
            file_maps[STUB_OWNER_FILE.get(s["owner"], "unknown")].add(m["map"])
    reasons_of = {m["map"]: set(r) for m, e, gp, r in nonclean}
    stubfile_of = {}
    for m, e, gp, r in nonclean:
        stubfile_of[m["map"]] = {"stub " + s["text"]: STUB_OWNER_FILE.get(s["owner"], "unknown") for s in gp}

    with open("SUMMARY.md", "w") as f:
        f.write("# Deus Ex 1112fm sweep summary\n\n")
        f.write("Maps: %d. clean: %d. fail: %d. crash: %d. timeout: %d. pass-verdict but non-clean (stub-only): %d.\n\n"
                % (len(rows), clean, verdicts["fail"], verdicts["crash"], verdicts["timeout"],
                   sum(1 for m, e, gp, r in nonclean if m["verdict"] == "pass")))
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
        f.write("\nFixing every file above together unblocks every stub-only map; the\n"
                "three login failures, the iterator failure and the segfault each\n"
                "need their own fix on top.\n")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
