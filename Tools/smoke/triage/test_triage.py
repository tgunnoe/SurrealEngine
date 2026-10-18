import dataclasses
import filecmp
import importlib.util
import json
import os
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

SMOKE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(SMOKE))
from triage import build, check, extract, repro  # noqa: E402
from triage.scenario import Scenario  # noqa: E402

DEUSEX_DIR = SMOKE / "scenarios" / "deusex"


def load_deusex():
    # Loaded by path: discovery puts this directory on sys.path, where a bare
    # "scenario" would be the package's own module.
    spec = importlib.util.spec_from_file_location("deusex_scenario", DEUSEX_DIR / "scenario.py")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod.DEUSEX


TOY = Scenario(
    label="Toy",
    triage_intro="Non-clean: {nonclean} of {total}.\n\n",
    triage_evidence="Evidence in sidecars.\n\n",
    stubs_footer="\nToy footer.\n",
    summary_footer="\nToy summary footer.\n",
    hierarchy={"Actor": ["Actor", "Object"], "Util": ["Util", "Object"],
               "Sub": ["Sub", "Actor", "Object"]},
    gameplay_roots=frozenset({"Actor"}),
    bare_owners={"Bare": "Util"},
    stub_owner_file={"Actor": "native/Actor.cpp"},
    signature_rules=((None, "contains", "boom", "script: boom"),
                     ("FATAL", "startswith", "signal", "crash: signal {last}")),
    error_owner={"script: boom": ("the boom owner", "none applies")},
    status_natives="status cite",
    bullet="- ",
    status_label="Status",
    verdict_template="verdict: {verdict}, exit code {exit_code}",
    reproduced_template=("reproduced: {reproduced} (second run exit code {exit_code}; "
                         "run1 `{run1}`; run2 `{run2}`)"),
    counts_template=("Maps: {maps}. clean: {clean}. fail: {fail}. crash: {crash}. "
                     "timeout: {timeout}. stub-only: {stub_only}."),
    exe="Engine",
    game_placeholder="<game>",
    exit_after_seconds=7,
    repro_timeout_seconds=20,
    repro_logdir="/nonexistent/repro",
)
ALL_OPTIONS = dict(
    row_count=8, screenshots_null=True, errors_in_section=True,
    gameplay_stubs_in_section=True, summary_counts_line=True, stubs_md_count=True,
    require_first_five=True, reproduced_exit_code=True, maps_loaded_equal=True,
    verdict_line_equal=True, map_match_case_insensitive=True)
TOY_ALL = dataclasses.replace(TOY, **ALL_OPTIONS)


def row(name, loaded=None):
    return {"map": name, "verdict": "pass", "exit_code": 0, "fps_avg": 30.0,
            "maps_loaded": [name] if loaded is None else loaded, "screenshot": None}


def stub(text="Actor.Foo", owner="Actor", cls="gameplay"):
    return {"text": text, "owner": owner, "class": cls}


def write_fixture(root):
    names = ["M1", "M2", "M3", "M4", "M5", "M6"]
    sweep = [row(n) for n in names] + [row("Cove"), row("Cased", ["cased"])]
    stubs = {n: [stub()] for n in names}
    stubs["Cove"] = []
    stubs["Cased"] = []
    errors = {n: {"errors": [], "fatal_after_summary": False} for n in stubs}
    errors["M1"]["errors"] = [{"kind": "SCRIPT_ERROR", "text": "boom here", "map_section": "M1"}]
    repro_rec = {"reproduced": "yes", "exit_code": 0, "first_line_run1": "a", "first_line_run2": "a"}
    repro_json = {n: repro_rec for n in names[:5]}
    data = {"sweep.json": {"maps": sweep},
            "stubs.json": {"owners": {"Actor": {"class": "gameplay", "reason": "Actor is a root"}},
                           "maps": stubs},
            "errors.json": errors, "repro.json": repro_json}
    for fn, obj in data.items():
        with open(os.path.join(root, fn), "w") as f:
            json.dump(obj, f)
    return data


class Fixture(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self._tmp.cleanup)
        self.data = os.path.join(self._tmp.name, "data")
        self.docs = os.path.join(self._tmp.name, "docs")
        os.mkdir(self.data)
        os.mkdir(self.docs)
        write_fixture(self.data)

    def generate(self, scn):
        build.build(scn, self.data, self.docs, os.path.join(self.data, "repro.json"))

    def edit_doc(self, name, old, new):
        p = os.path.join(self.docs, name)
        with open(p) as f:
            text = f.read()
        self.assertIn(old, text)
        with open(p, "w") as f:
            f.write(text.replace(old, new, 1))

    def edit_data(self, name, fn):
        p = os.path.join(self.data, name)
        with open(p) as f:
            obj = json.load(f)
        fn(obj)
        with open(p, "w") as f:
            json.dump(obj, f)

    def problems(self, scn):
        return check.check(scn, self.data, self.docs)[2]

    def only(self, **opts):
        # Case-insensitive matching stays on so the "Cased" row is clean
        # in every fixture except the one testing that option.
        return dataclasses.replace(TOY, map_match_case_insensitive=True, **opts)

    def assert_option(self, option, value, mutate, marker):
        """The mutation is flagged with the option on, for the marker's reason,
        and not flagged with the option off."""
        on = self.only(**{option: value})
        self.generate(on)
        self.assertEqual(self.problems(on), [], "fixture must pass with %s on" % option)
        mutate()
        flagged = self.problems(on)
        self.assertTrue(any(marker in p for p in flagged), (option, flagged))
        off = self.only(**{option: None if option == "row_count" else False})
        self.assertEqual(self.problems(off), [], "mutation must pass with %s off" % option)


class TestClassify(unittest.TestCase):
    def test_hierarchy(self):
        self.assertEqual(extract.classify(TOY, "Actor")[0], "gameplay")
        self.assertEqual(extract.classify(TOY, "Sub")[0], "gameplay")
        self.assertEqual(extract.classify(TOY, "Util")[0], "non-gameplay")
        self.assertEqual(extract.classify(TOY, "Nope")[0], "unclassified")

    def test_bare_owner(self):
        self.assertEqual(extract.owner_of(TOY, "Bare(1)"), "Util")
        self.assertEqual(extract.owner_of(TOY, "Actor.Foo"), "Actor")
        self.assertEqual(extract.owner_of(TOY, "Unlisted"), "")
        self.assertEqual(extract.classify(TOY, extract.owner_of(TOY, "Unlisted"))[0], "unclassified")


class TestExtract(unittest.TestCase):
    def test_extract_round_trip(self):
        with tempfile.TemporaryDirectory() as logs, tempfile.TemporaryDirectory() as out:
            frame = " at /x/bin(foo+0x1f) [0xdeadbeef]"
            with open(os.path.join(logs, "M1.log"), "w") as f:
                f.write("MAP M1\nSTUB Actor.Foo\nSTUB Actor.Foo\nSTUB Bare(2)\n"
                        "SCRIPT_ERROR boom Native call stack:" + frame + "\n"
                        "SUMMARY done\nFATAL signal 11\n")
            with open(os.path.join(logs, "notes.txt"), "w") as f:
                f.write("ignored")
            extract.extract(TOY, logs, out)
            with open(os.path.join(out, "stubs.json")) as f:
                stubs = json.load(f)
            with open(os.path.join(out, "errors.json")) as f:
                errs = json.load(f)
        self.assertEqual([s["text"] for s in stubs["maps"]["M1"]], ["Actor.Foo", "Bare(2)"])
        self.assertEqual([s["class"] for s in stubs["maps"]["M1"]], ["gameplay", "non-gameplay"])
        self.assertEqual(stubs["owners"]["Util"]["class"], "non-gameplay")
        self.assertEqual(errs["M1"]["errors"][0], {"kind": "SCRIPT_ERROR", "text": "boom", "map_section": "M1"})
        self.assertTrue(errs["M1"]["fatal_after_summary"])
        self.assertEqual(set(stubs["maps"]), {"M1"})

    def test_extract_counts_warnings(self):
        with tempfile.TemporaryDirectory() as logs, tempfile.TemporaryDirectory() as out:
            with open(os.path.join(logs, "M1.log"), "w") as f:
                f.write("MAP M1\nSCRIPT_WARNING Accessed None 'A'\nSCRIPT_WARNING Accessed None 'A'\n"
                        "SCRIPT_WARNING Accessed None 'B'\nSUMMARY done\n")
            with open(os.path.join(logs, "M2.log"), "w") as f:
                f.write("MAP M2\nSUMMARY done\n")
            extract.extract(TOY, logs, out)
            with open(os.path.join(out, "errors.json")) as f:
                errs = json.load(f)
        # Every line counts, as in the sweep's own tally, so repeated text is not collapsed.
        self.assertEqual(errs["M1"].get("warnings"), 3)
        self.assertEqual(errs["M2"].get("warnings"), 0)
        self.assertEqual(errs["M1"]["errors"], [])

    def test_strip_native_stack(self):
        self.assertEqual(extract.strip_native_stack("a b at /x(f+0x1) [0x2] at /y(g+0x3) [0x4]"), "a b")


class TestReproCompare(unittest.TestCase):
    def logs(self, old, new):
        d = tempfile.TemporaryDirectory()
        self.addCleanup(d.cleanup)
        paths = []
        for i, body in enumerate((old, new)):
            p = os.path.join(d.name, "%d.log" % i)
            with open(p, "w") as f:
                f.write(body)
            paths.append(p)
        return paths

    def test_same_first_failing_line(self):
        v, a, b = repro.compare(TOY, *self.logs("SCRIPT_ERROR x 0x10\n", "SCRIPT_ERROR x 0x20\nSTUB Actor.Q\n"))
        self.assertEqual((v, a, b), ("yes", "SCRIPT_ERROR x 0x?", "SCRIPT_ERROR x 0x?"))

    def test_different_first_failing_line(self):
        self.assertEqual(repro.compare(TOY, *self.logs("SCRIPT_ERROR x\n", "SCRIPT_ERROR y\n"))[0], "flaky")

    def test_error_only_in_first_run_is_no(self):
        self.assertEqual(repro.compare(TOY, *self.logs("SCRIPT_ERROR x\n", "MAP M\n"))[0], "no")

    def test_error_only_in_second_run_is_flaky(self):
        self.assertEqual(repro.compare(TOY, *self.logs("MAP M\n", "SCRIPT_ERROR x\n"))[0], "flaky")

    def test_stub_set_fallback(self):
        same = repro.compare(TOY, *self.logs("STUB Actor.B\nSTUB Actor.A\nSTUB Util.Z\n",
                                             "STUB Actor.A\nSTUB Actor.B\n"))
        self.assertEqual(same, ("yes", "stubs:Actor.A,Actor.B", "stubs:Actor.A,Actor.B"))
        self.assertEqual(repro.compare(TOY, *self.logs("STUB Actor.A\n", "STUB Actor.B\n"))[0], "flaky")


class TestCheckOptions(Fixture):
    def test_all_options_pass_on_good_fixture(self):
        self.generate(TOY_ALL)
        self.assertEqual(check.check(TOY_ALL, self.data, self.docs), (6, 6, []))
        self.assertEqual(check.format_result(6, 6, []), "sections=6 nonclean=6 problems=0")

    def test_row_count(self):
        self.assert_option("row_count", 8, lambda: self.edit_data(
            "sweep.json", lambda o: o["maps"].pop()), "expected 8 rows")

    def test_screenshots_null(self):
        self.assert_option("screenshots_null", True, lambda: self.edit_data(
            "sweep.json", lambda o: o["maps"][0].update(screenshot="x.png")), "screenshot")

    def test_errors_in_section(self):
        self.assert_option("errors_in_section", True, lambda: self.edit_doc(
            "TRIAGE.md", "`SCRIPT_ERROR boom here`", "`SCRIPT_ERROR other`"), "error line not verbatim")

    def test_gameplay_stubs_in_section(self):
        self.assert_option("gameplay_stubs_in_section", True, lambda: self.edit_doc(
            "TRIAGE.md", "`Actor.Foo`", "`Actor.Other`"), "gameplay stub absent")

    def test_summary_counts_line(self):
        self.assert_option("summary_counts_line", True, lambda: self.edit_doc(
            "SUMMARY.md", "fail: 0", "fail: 9"), "counts line differs")

    def test_stubs_md_count(self):
        self.assert_option("stubs_md_count", True, lambda: self.edit_doc(
            "stubs.md", "Rows below: 1", "Rows below: 2"), "stubs.md: counts")

    def test_require_first_five(self):
        self.assert_option("require_first_five", True, lambda: self.edit_doc(
            "TRIAGE.md", "- reproduced: yes (second run exit code 0; run1 `a`; run2 `a`)\n\n## M3",
            "- reproduced: not-run\n\n## M3"), "must be re-run")

    def test_reproduced_exit_code(self):
        self.assert_option("reproduced_exit_code", True, lambda: self.edit_doc(
            "TRIAGE.md", "second run exit code 0", "second run"), "exit code")

    def test_maps_loaded_equal(self):
        self.assert_option("maps_loaded_equal", True, lambda: self.edit_doc(
            "TRIAGE.md", '- maps_loaded: ["M2"]', '- maps_loaded: ["Other"]'), "maps_loaded")

    def test_verdict_line_equal(self):
        self.assert_option("verdict_line_equal", True, lambda: self.edit_doc(
            "TRIAGE.md", "verdict: pass, exit code 0", "verdict: fail, exit code 1"), "verdict line")

    def test_map_match_case_insensitive(self):
        on = self.only()
        self.generate(on)
        self.assertEqual(self.problems(on), [])
        with open(os.path.join(self.docs, "TRIAGE.md")) as f:
            self.assertNotIn("## Cased", f.read())
        # Exact matching treats the same data as non-clean, so the on-docs miss a section.
        self.assertTrue(any("section set differs" in p for p in self.problems(TOY)))

    def test_section_removed_is_flagged_by_default(self):
        base = self.only()
        self.generate(base)
        self.edit_doc("TRIAGE.md", "## M2", "## Renamed")
        self.assertTrue(any("section set differs" in p for p in self.problems(base)))


class TestWarningsPolicy(Fixture):
    """Warnings are counted and shown; they make a map non-clean only when the
    scenario asks, and an absent count is unknown, never zero."""

    WITH_TOTAL = TOY.counts_template + " warnings: {warnings}."

    def record(self, **counts):
        def apply(o):
            for m in o["maps"]:
                if m["map"] in counts:
                    m["script_warnings"] = counts[m["map"]]
        self.edit_data("sweep.json", apply)

    def record_all(self):
        def apply(o):
            for i, m in enumerate(o["maps"]):
                m["script_warnings"] = i
        self.edit_data("sweep.json", apply)
        return sum(range(8))

    def failing_on_warnings(self, **opts):
        # Asserted, so a scenario without the option fails the test instead of
        # raising from dataclasses.replace.
        self.assertIn("warnings_fail_clean", {f.name for f in dataclasses.fields(Scenario)})
        return self.only(warnings_fail_clean=True, **opts)

    def section(self, name):
        with open(os.path.join(self.docs, "TRIAGE.md")) as f:
            return f.read().split("## %s\n" % name, 1)[1].split("\n## ", 1)[0]

    def test_option_defaults_off(self):
        self.assertIs(getattr(Scenario(), "warnings_fail_clean", None), False)

    def test_warnings_only_map_is_clean_by_default(self):
        self.record(Cove=3)
        scn = self.only()
        self.generate(scn)
        with open(os.path.join(self.docs, "TRIAGE.md")) as f:
            self.assertNotIn("## Cove", f.read())
        self.assertEqual(check.check(scn, self.data, self.docs), (6, 6, []))

    def test_warnings_only_map_is_nonclean_when_option_set(self):
        self.record(Cove=3)
        scn = self.failing_on_warnings()
        self.generate(scn)
        self.assertEqual(check.check(scn, self.data, self.docs), (7, 7, []))
        self.assertIn("- warnings: 3\n", self.section("Cove"))
        with open(os.path.join(self.docs, "SUMMARY.md")) as f:
            self.assertIn("| warnings | 1 | Cove |", f.read())

    def test_zero_count_stays_clean_when_option_set(self):
        self.record(Cove=0)
        scn = self.failing_on_warnings()
        self.generate(scn)
        self.assertEqual(check.check(scn, self.data, self.docs), (6, 6, []))

    def test_absent_count_never_flags_a_map(self):
        scn = self.failing_on_warnings()
        self.generate(scn)
        with open(os.path.join(self.docs, "TRIAGE.md")) as f:
            self.assertNotIn("## Cove", f.read())
        self.assertEqual(check.check(scn, self.data, self.docs), (6, 6, []))

    def test_section_warnings_line_recorded_and_absent(self):
        self.record(M1=4)
        self.generate(self.only())
        self.assertIn("- warnings: 4\n", self.section("M1"))
        self.assertIn("- warnings: not recorded\n", self.section("M2"))

    def test_summary_total_when_every_row_records(self):
        total = self.record_all()
        scn = self.only(summary_counts_line=True, counts_template=self.WITH_TOTAL)
        self.generate(scn)
        with open(os.path.join(self.docs, "SUMMARY.md")) as f:
            self.assertIn(" warnings: %d.\n" % total, f.read())
        self.assertEqual(self.problems(scn), [])
        self.edit_doc("SUMMARY.md", " warnings: %d." % total, " warnings: %d." % (total + 1))
        self.assertTrue(any("counts line differs" in p for p in self.problems(scn)))

    def test_summary_total_not_recorded_when_any_row_lacks_it(self):
        self.record(M1=4)
        scn = self.only(summary_counts_line=True, counts_template=self.WITH_TOTAL)
        self.generate(scn)
        with open(os.path.join(self.docs, "SUMMARY.md")) as f:
            self.assertIn(" warnings: not recorded.\n", f.read())
        self.assertEqual(self.problems(scn), [])
        # A partial sum would read as a measurement; the checker must reject it.
        self.edit_doc("SUMMARY.md", " warnings: not recorded.", " warnings: 4.")
        self.assertTrue(any("counts line differs" in p for p in self.problems(scn)))


class TestReproCommand(unittest.TestCase):
    """The engine command a re-run starts and the command the document shows."""

    def command(self, scn):
        with tempfile.TemporaryDirectory() as tmp:
            old = os.path.join(tmp, "old")
            os.mkdir(old)
            open(os.path.join(old, "M1.log"), "w").close()
            scn = dataclasses.replace(scn, repro_logdir=os.path.join(tmp, "new"))
            seen = []

            def fake_run(cmd, **kwargs):
                seen.append(cmd)
                open(next(a for a in cmd if a.startswith("--smoke-log=")).split("=", 1)[1], "w").close()
                return mock.Mock(returncode=0)

            with mock.patch.object(repro.subprocess, "run", side_effect=fake_run):
                repro.run(scn, "Engine", "<game>", old, os.path.join(tmp, "repro.json"), ["M1"])
        self.assertEqual(len(seen), 1)
        return seen[0]

    def test_options_are_joined_to_the_url_and_exec_follows(self):
        cmd = self.command(dataclasses.replace(TOY, url_options=("?NumBots=4", "?Foo=1"), exec="5:Fire"))
        self.assertEqual(cmd[4:], ["--url=M1?NumBots=4?Foo=1", "--exec=5:Fire", "<game>"])

    def test_no_exec_means_no_flag(self):
        cmd = self.command(dataclasses.replace(TOY, url_options=("?NumBots=4",)))
        self.assertEqual(cmd[4:], ["--url=M1?NumBots=4", "<game>"])

    def test_plain_scenario_command_is_unchanged(self):
        self.assertEqual(self.command(TOY)[4:], ["--url=M1", "<game>"])


class TestRequireMatchStart(Fixture):
    """A map whose match never started is not clean, when the scenario says
    so; the flag is recorded by the sweep and an absent flag is unknown."""

    OWNER = {"match did not start": ("the match owner", "none applies")}
    COUNTS = TOY.counts_template + " matches started: {matches_started}."

    def record(self, **started):
        def apply(o):
            for m in o["maps"]:
                if m["map"] in started:
                    m["match_started"] = started[m["map"]]
        self.edit_data("sweep.json", apply)

    def record_all(self, **overrides):
        names = ["M1", "M2", "M3", "M4", "M5", "M6", "Cove", "Cased"]
        self.record(**{n: overrides.get(n, True) for n in names})

    def matching(self, **opts):
        return self.only(require_match_start=True, error_owner={**TOY.error_owner, **self.OWNER}, **opts)

    def section(self, name):
        with open(os.path.join(self.docs, "TRIAGE.md")) as f:
            return f.read().split("## %s\n" % name, 1)[1].split("\n## ", 1)[0]

    def read(self, name):
        with open(os.path.join(self.docs, name)) as f:
            return f.read()

    def test_options_default_off(self):
        self.assertIs(Scenario().require_match_start, False)
        self.assertEqual(Scenario().exec, "")

    def test_unstarted_match_is_nonclean_with_its_reason(self):
        self.record_all(Cove=False)
        scn = self.matching()
        self.generate(scn)
        self.assertEqual(check.check(scn, self.data, self.docs), (7, 7, []))
        cove = self.section("Cove")
        self.assertIn("- match_started: false\n", cove)
        self.assertIn("- owner: the match owner\n", cove)
        self.assertIn("| match did not start | 1 | Cove |", self.read("SUMMARY.md"))

    def test_started_matches_stay_clean(self):
        self.record_all()
        scn = self.matching()
        self.generate(scn)
        self.assertNotIn("## Cove", self.read("TRIAGE.md"))
        self.assertEqual(check.check(scn, self.data, self.docs), (6, 6, []))

    def test_absent_flag_never_flags_a_map(self):
        scn = self.matching()
        self.generate(scn)
        self.assertNotIn("## Cove", self.read("TRIAGE.md"))
        self.assertEqual(check.check(scn, self.data, self.docs), (6, 6, []))

    def test_flag_is_ignored_when_the_option_is_off(self):
        self.record_all(Cove=False)
        scn = self.only()
        self.generate(scn)
        self.assertNotIn("## Cove", self.read("TRIAGE.md"))
        self.assertNotIn("match_started", self.read("TRIAGE.md"))
        self.assertEqual(check.check(scn, self.data, self.docs), (6, 6, []))

    def test_checker_compares_the_section_line_with_the_row(self):
        self.record_all(Cove=False)
        scn = self.matching()
        self.generate(scn)
        self.edit_doc("TRIAGE.md", "- match_started: false", "- match_started: true")
        self.assertTrue(any("match_started line" in p for p in self.problems(scn)))

    def test_checker_notices_documents_built_without_the_option(self):
        self.record_all(Cove=False)
        self.generate(self.only())
        self.assertTrue(any("section set differs" in p for p in self.problems(self.matching())))

    def test_matches_started_count(self):
        self.record_all(Cove=False)
        scn = self.only(summary_counts_line=True, counts_template=self.COUNTS)
        self.generate(scn)
        self.assertIn(" matches started: 7.\n", self.read("SUMMARY.md"))
        self.assertEqual(self.problems(scn), [])
        self.edit_doc("SUMMARY.md", " matches started: 7.", " matches started: 8.")
        self.assertTrue(any("counts line differs" in p for p in self.problems(scn)))

    def test_matches_started_counts_only_recorded_true(self):
        self.record(M1=True, M2=True, M3=False)
        scn = self.only(summary_counts_line=True, counts_template=self.COUNTS)
        self.generate(scn)
        self.assertIn(" matches started: 2.\n", self.read("SUMMARY.md"))
        self.assertEqual(self.problems(scn), [])


class TestReproLine(Fixture):
    """The command a section shows is the one a shell can run, and the
    checker holds it to the scenario's options."""

    def scenario(self):
        return self.only(url_options=("?NumBots=4",), exec="5:Fire")

    def test_line_carries_options_quoted_and_exec(self):
        scn = self.scenario()
        self.generate(scn)
        with open(os.path.join(self.docs, "TRIAGE.md")) as f:
            body = f.read()
        self.assertIn("- repro: `Engine --autostart --exit-after=7 --smoke-log=/nonexistent/repro/M1.log "
                      "'--url=M1?NumBots=4' --exec=5:Fire \"<game>\"`\n", body)
        self.assertEqual(self.problems(scn), [])

    def test_checker_flags_a_line_without_the_exec(self):
        scn = self.scenario()
        self.generate(scn)
        self.edit_doc("TRIAGE.md", " --exec=5:Fire", "")
        self.assertTrue(any("M1: missing repro command" in p for p in self.problems(scn)))

    def test_checker_flags_a_line_without_the_url_options(self):
        scn = self.scenario()
        self.generate(scn)
        self.edit_doc("TRIAGE.md", "'--url=M1?NumBots=4'", "--url=M1")
        self.assertTrue(any("M1: missing repro command" in p for p in self.problems(scn)))


class TestDeusExByteIdentity(unittest.TestCase):
    def test_regenerates_committed_documents(self):
        scn = load_deusex()
        with tempfile.TemporaryDirectory() as out:
            build.build(scn, str(DEUSEX_DIR), out, str(DEUSEX_DIR / "repro.json"))
            for name in ("TRIAGE.md", "stubs.md", "SUMMARY.md"):
                self.assertTrue(filecmp.cmp(os.path.join(out, name), DEUSEX_DIR / name, shallow=False), name)
            _, _, problems = check.check(scn, str(DEUSEX_DIR), out)
        self.assertEqual(problems, [])

    def test_scenario_has_no_defaults_naming_a_game(self):
        for f in dataclasses.fields(Scenario):
            if f.default is not dataclasses.MISSING:
                self.assertNotIn("Deus", repr(f.default))


if __name__ == "__main__":
    unittest.main()
