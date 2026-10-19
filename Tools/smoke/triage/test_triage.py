import contextlib
import dataclasses
import filecmp
import importlib.util
import io
import json
import os
import re
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

SMOKE = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(SMOKE))
from triage import build, check, extract, repro  # noqa: E402
from triage.scenario import Scenario  # noqa: E402

SCENARIOS = SMOKE / "scenarios"


def load_scenario(name):
    # Loaded by path: discovery puts this directory on sys.path, where a bare
    # "scenario" would be the package's own module.
    spec = importlib.util.spec_from_file_location(
        "scenario_" + name.replace("-", "_"), SCENARIOS / name / "scenario.py")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    found = [v for v in vars(mod).values() if isinstance(v, Scenario)]
    assert len(found) == 1, (name, found)
    return found[0]


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
    signature_rules=((None, "contains", "boom", "script: boom"),),
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

    def read(self, name):
        with open(os.path.join(self.docs, name)) as f:
            return f.read()

    def section(self, name):
        return self.read("TRIAGE.md").split("## %s\n" % name, 1)[1].split("\n## ", 1)[0]

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

    def test_extract_records_warning_sites(self):
        with tempfile.TemporaryDirectory() as logs, tempfile.TemporaryDirectory() as out:
            with open(os.path.join(logs, "M1.log"), "w") as f:
                f.write("MAP M1\nSCRIPT_WARNING Accessed None in A.b\n"
                        "SCRIPT_WARNING  Accessed   None in A.b\n"
                        "SCRIPT_WARNING Accessed None in C.d at 0xDEAD\n"
                        "SCRIPT_WARNING Accessed None in C.d at 0xbeef\nSUMMARY done\n")
            with open(os.path.join(logs, "M2.log"), "w") as f:
                f.write("MAP M2\nSUMMARY done\n")
            extract.extract(TOY, logs, out)
            with open(os.path.join(out, "errors.json")) as f:
                errs = json.load(f)
        # One site per text once whitespace is collapsed and addresses are masked.
        sites = {"Accessed None in A.b": 2, "Accessed None in C.d at 0x?": 2}
        self.assertEqual(errs["M1"]["warning_sites"], sites)
        self.assertEqual(list(errs["M1"]["warning_sites"]), sorted(sites))
        self.assertEqual(errs["M2"]["warning_sites"], {})
        # Every warning line lands on exactly one site.
        for rec in errs.values():
            self.assertEqual(rec["warnings"], sum(rec["warning_sites"].values()))

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

    def test_stub_only_first_run_whose_stub_vanishes_is_no(self):
        # A gameplay stub makes a run non-clean in the checker, so losing it is a "no".
        self.assertEqual(repro.compare(TOY, *self.logs("STUB Actor.A\n", "MAP M\n")),
                         ("no", "stubs:Actor.A", "stubs:"))
        started = self.STARTED
        self.assertEqual(repro.compare(self.MATCH, *self.logs(started + "STUB Actor.A\n", started))[0], "no")

    def test_stub_only_in_second_run_is_flaky(self):
        self.assertEqual(repro.compare(TOY, *self.logs("MAP M\n", "STUB Actor.A\n"))[0], "flaky")

    def test_non_start_with_a_stub_then_start_without_is_no(self):
        v, a, b = repro.compare(self.MATCH, *self.logs(self.UNSTARTED + "STUB Actor.A\n", self.STARTED))
        self.assertEqual((v, a, b), ("no", "stubs:Actor.A; match_started:false", "stubs:; match_started:true"))

    def test_stub_set_fallback(self):
        same = repro.compare(TOY, *self.logs("STUB Actor.B\nSTUB Actor.A\nSTUB Util.Z\n",
                                             "STUB Actor.A\nSTUB Actor.B\n"))
        self.assertEqual(same, ("yes", "stubs:Actor.A,Actor.B", "stubs:Actor.A,Actor.B"))
        self.assertEqual(repro.compare(TOY, *self.logs("STUB Actor.A\n", "STUB Actor.B\n"))[0], "flaky")

    # With require_match_start a non-start is a failure like a failing line,
    # judged by the sweep's own rule; without it the flag is not looked at.
    STARTED = "STATE 0.0 PlayerWalking\nEXEC 5 Fire\n"
    UNSTARTED = "STATE 0.0 PlayerWaiting\nEXEC 5 Fire\n"
    MATCH = dataclasses.replace(TOY, require_match_start=True)

    def test_non_start_in_both_runs_is_yes(self):
        self.assertEqual(repro.compare(self.MATCH, *self.logs(self.UNSTARTED, self.UNSTARTED))[0], "yes")

    def test_non_start_then_start_is_no(self):
        v, a, b = repro.compare(self.MATCH, *self.logs(self.UNSTARTED, self.STARTED))
        self.assertEqual((v, a, b), ("no", "stubs:; match_started:false", "stubs:; match_started:true"))

    def test_start_then_non_start_is_flaky(self):
        v, a, b = repro.compare(self.MATCH, *self.logs(self.STARTED, self.UNSTARTED))
        self.assertEqual((v, a, b), ("flaky", "stubs:; match_started:true", "stubs:; match_started:false"))

    def test_flag_is_ignored_without_the_option(self):
        self.assertEqual(repro.compare(TOY, *self.logs(self.UNSTARTED, self.STARTED)),
                         ("yes", "stubs:", "stubs:"))

    def test_stub_sets_are_still_compared_when_the_matches_agree(self):
        self.assertEqual(repro.compare(self.MATCH, *self.logs(self.STARTED + "STUB Actor.A\n",
                                                              self.STARTED + "STUB Actor.B\n"))[0], "flaky")

    def test_every_combination_follows_the_rule(self):
        # A run is its first failing line (none, x or y) and whether its match started.
        runs = [(line, started) for line in (None, "x", "y") for started in (False, True)]

        def body(run):
            line, started = run
            return ("" if line is None else "SCRIPT_ERROR %s\n" % line) + (self.STARTED if started else self.UNSTARTED)

        def unclean(run):
            return run[0] is not None or not run[1]

        for old in runs:
            for new in runs:
                want = "yes" if old == new else "no" if unclean(old) and not unclean(new) else "flaky"
                with self.subTest(old=old, new=new):
                    v, a, b = repro.compare(self.MATCH, *self.logs(body(old), body(new)))
                    self.assertEqual(v, want)
                    self.assertTrue(str(a).endswith("; match_started:" + str(old[1]).lower()), a)
                    self.assertTrue(str(b).endswith("; match_started:" + str(new[1]).lower()), b)

    def test_start_follows_the_rule(self):
        bodies = [
            ("STATE 0.0 PlayerWalking at (1.0, 2.0, 3.0)\nEXEC 5 Fire\n", True),
            ("STATE 0.0 PlayerWaiting\nEXEC 5 Fire\nSTATE 6.0 PlayerWalking\n", True),
            ("STATE 0.0 PlayerWalking\nSTATE 3.0 PlayerWaiting\nEXEC 5 Fire\n", False),
            ("STATE 0.0 PlayerWalking\nEXEC_FAILED 5 Fire\nSTATE 6.0 PlayerWalking\n", False),
            ("STATE 0.0 PlayerWalking\n", False),
        ]
        for text, want in bodies:
            old, new = self.logs(text, text)
            with self.subTest(text=text):
                _, a, b = repro.compare(self.MATCH, old, new)
                self.assertTrue(str(a).endswith("; match_started:" + str(want).lower()), a)


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


def table_rows(text, heading):
    """The cells of each row of the table under `## heading`, header and rule dropped."""
    body = text.split("## %s\n" % heading, 1)[1].split("\n## ", 1)[0]
    rows = [line for line in body.splitlines() if line.startswith("|")]
    return [[c.strip() for c in re.split(r"(?<!\\)\|", line.strip()[1:-1])] for line in rows[2:]]


def fatal(text):
    return {"kind": "FATAL", "text": text, "map_section": "M1"}


def boom():
    return {"kind": "SCRIPT_ERROR", "text": "boom here", "map_section": "M1"}


class TestRendering(Fixture):
    DOCS = ("TRIAGE.md", "stubs.md", "SUMMARY.md")

    def make_clean(self):
        self.edit_data("stubs.json", lambda o: o["maps"].update({n: [] for n in o["maps"]}))
        self.edit_data("errors.json", lambda o: [r.update(errors=[]) for r in o.values()])

    def crash(self, exit_code, errors, verdict="crash"):
        self.edit_data("sweep.json", lambda o: o["maps"][0].update(verdict=verdict, exit_code=exit_code))
        self.edit_data("errors.json", lambda o: o["M1"].update(errors=errors))

    def signatures(self, scn=None):
        self.generate(scn or self.only())
        return {r[0]: r for r in table_rows(self.read("SUMMARY.md"), "Failure signatures")}

    def assert_one_newline(self, scn):
        self.generate(scn)
        for name in self.DOCS:
            text = self.read(name)
            self.assertTrue(text.endswith("\n") and text.rstrip("\n") + "\n" == text, (name, text[-20:]))

    def test_every_document_ends_with_exactly_one_newline(self):
        padded = dict(stubs_footer="\nFoot.\n\n\n", summary_footer="\nFoot.\n\n\n")
        bare = dict(stubs_footer="", summary_footer="")
        for label, opts, clean in (("default", {}, False), ("padded footers", padded, False),
                                   ("no footers", bare, False), ("no sections", {}, True),
                                   ("no sections, no evidence", dict(triage_evidence="", **bare), True)):
            with self.subTest(label):
                if clean:
                    self.make_clean()
                self.assert_one_newline(self.only(**opts))

    def test_empty_signature_and_candidate_tables_render_a_none_row(self):
        self.make_clean()
        self.generate(self.only())
        summary = self.read("SUMMARY.md")
        self.assertIn("| Signature | Maps | Affected |\n| :--- | ---: | :--- |\n| none | | |\n", summary)
        self.assertIn("| Rank | Candidate | Fully unblocks | Partial |\n"
                      "| ---: | :--- | ---: | ---: |\n| none | | | |\n", summary)

    def test_non_empty_tables_have_no_none_row(self):
        self.generate(self.only())
        summary = self.read("SUMMARY.md")
        for heading in ("Failure signatures", "Ranked fix candidates"):
            self.assertNotIn("none", [r[0] for r in table_rows(summary, heading)], heading)

    def test_summary_check_sentence_names_the_check_command(self):
        self.generate(self.only())
        paragraph = self.read("SUMMARY.md").split("Check:", 1)[1].split("\n\n", 1)[0]
        self.assertIn("`check`", paragraph)
        self.assertNotIn("check_triage", paragraph)

    def test_a_crash_row_yields_one_crash_signature(self):
        long_text = "out of memory at 0xDEADBEEF " + "y" * 100
        cases = [
            ("signal exit code", -11, [boom(), fatal("signal 11")], "crash: FATAL signal 11"),
            ("signal beats an earlier fatal line", -6, [fatal("terminate: bad"), fatal("signal 6")],
             "crash: FATAL signal 6"),
            ("signal with no recorded fatal line", -6, [boom()], "crash: FATAL signal 6"),
            ("other fatal line, masked and cut", 1, [fatal(long_text)],
             "crash: FATAL " + ("out of memory at 0x? " + "y" * 100)[:80]),
            ("no fatal line and no signal", 3, [boom()], "crash: exit 3"),
        ]
        for label, exit_code, errors, want in cases:
            with self.subTest(label):
                self.crash(exit_code, errors)
                sigs = self.signatures()
                self.assertEqual([s for s in sigs if "crash" in s or "FATAL" in s], [want])
                self.assertEqual(sigs[want][1:], ["1", "M1"])
                # The recorded lines that are not fatal keep their own signatures.
                self.assertEqual("script: boom" in sigs, boom() in errors)

    def test_a_fatal_line_in_a_non_crash_row_keeps_the_default_form(self):
        # signature_rules do not apply to FATAL lines, even a rule that would match.
        rule = ("FATAL", "startswith", "signal", "crash: signal {last}")
        scn = self.only(signature_rules=TOY.signature_rules + (rule,))
        for text, want in (("signal 11 at 0xAB", "FATAL: signal 11 at 0x?"),
                           ("boom in terminate", "FATAL: boom in terminate")):
            with self.subTest(text):
                self.crash(1, [fatal(text)], verdict="fail")
                sigs = self.signatures(scn)
                self.assertEqual([s for s in sigs if "FATAL" in s or "crash" in s or "boom" in s], [want])

    def test_crash_owner_and_candidate_use_the_derived_signature(self):
        scn = self.only(error_owner={"crash: FATAL signal 11": ("the crash owner", "none applies")})
        self.crash(-11, [fatal("signal 11")])
        self.generate(scn)
        section = self.section("M1")
        self.assertIn("- owner: the crash owner", section)
        self.assertNotIn("unattributed", section)
        candidates = [r[1] for r in table_rows(self.read("SUMMARY.md"), "Ranked fix candidates")]
        self.assertIn("fix signature: crash: FATAL signal 11", candidates)
        self.assertFalse([c for c in candidates if "verdict crash" in c])


class TestWarningSites(Fixture):
    def set_sites(self, **sites):
        def apply(o):
            for name, counts in sites.items():
                o[name]["warning_sites"] = counts
        self.edit_data("errors.json", apply)

    def test_table_lists_every_site_by_lines_then_name(self):
        # Cove is a clean row: the table covers every row, not only the sections.
        self.set_sites(M1={"B site": 3, "A site": 3, "C|pipe": 1}, M2={"B site": 2, "D site": 5},
                       Cove={"A site": 1})
        self.generate(self.only())
        summary = self.read("SUMMARY.md")
        self.assertEqual(table_rows(summary, "Warning sites"),
                         [["B site", "5", "2"], ["D site", "5", "1"], ["A site", "4", "2"],
                          ["C\\|pipe", "1", "1"]])
        self.assertIn("| Site | Lines | Maps |\n| :--- | ---: | ---: |\n| B site | 5 | 2 |\n", summary)
        self.assertIn("| C\\|pipe | 1 | 1 |\n", summary)

    def test_table_sits_between_the_check_paragraph_and_the_signatures(self):
        self.generate(self.only())
        summary = self.read("SUMMARY.md")
        self.assertLess(summary.index("Check:"), summary.index("## Warning sites"))
        self.assertLess(summary.index("## Warning sites"), summary.index("## Failure signatures"))

    def test_rows_without_sites_render_a_none_row(self):
        self.generate(self.only())
        self.assertIn("| Site | Lines | Maps |\n| :--- | ---: | ---: |\n| none | | |\n", self.read("SUMMARY.md"))

    def test_sections_keep_the_warnings_count(self):
        self.set_sites(M1={"A site": 4})
        self.edit_data("sweep.json", lambda o: o["maps"][0].update(script_warnings=4))
        self.generate(self.only())
        self.assertIn("- warnings: 4\n", self.section("M1"))
        self.assertNotIn("A site", self.read("TRIAGE.md"))


class TestEntryPoint(unittest.TestCase):
    """The module's command line: a scenario directory, a command, its arguments."""

    SCENARIO = (
        "from triage.scenario import Scenario\n"
        "LABEL = 'Entry'\n"
        "ENTRY = Scenario(label=LABEL, exe='Engine', game_placeholder='<game>', bullet='- ',\n"
        "                 status_label='Status',\n"
        "                 exit_after_seconds=7, repro_timeout_seconds=20, repro_logdir=REPRO_LOGDIR,\n"
        "                 triage_intro='Non-clean: {nonclean} of {total}.\\n\\n',\n"
        "                 verdict_template='verdict: {verdict}, exit code {exit_code}',\n"
        "                 reproduced_template='reproduced: {reproduced}')\n")

    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.root = Path(tmp.name)
        self.scn_dir = self.root / "scn"
        self.logs = self.root / "logs"
        self.cwd = self.root / "cwd"
        for d in (self.scn_dir, self.logs, self.cwd):
            d.mkdir()
        self.write_scenario(self.SCENARIO)
        (self.logs / "M1.log").write_text("MAP M1\nSCRIPT_ERROR boom\nSUMMARY done\n")
        (self.logs / "M2.log").write_text("MAP M2\nSUMMARY done\n")
        rows = [{"map": "M1", "verdict": "fail", "exit_code": 1, "fps_avg": 30.0, "maps_loaded": ["M1"]},
                {"map": "M2", "verdict": "pass", "exit_code": 0, "fps_avg": 30.0, "maps_loaded": ["M2"]}]
        (self.scn_dir / "sweep.json").write_text(json.dumps({"maps": rows}))
        (self.scn_dir / "repro.json").write_text("{}")
        old = os.getcwd()
        os.chdir(self.cwd)
        self.addCleanup(os.chdir, old)

    def write_scenario(self, source):
        (self.scn_dir / "scenario.py").write_text(
            source.replace("REPRO_LOGDIR", repr(str(self.root / "rerun"))))

    def run_cli(self, *argv):
        from triage import cli
        out, err = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            code = cli.main([str(a) for a in argv])
        return code, out.getvalue(), err.getvalue()

    def extract_and_build(self):
        self.assertEqual(self.run_cli(self.scn_dir, "extract", os.path.relpath(self.logs))[0], 0)
        self.assertEqual(self.run_cli(self.scn_dir, "build", "repro.json")[0], 0)

    def test_extract_resolves_the_log_dir_against_the_working_dir(self):
        code, _, err = self.run_cli(self.scn_dir, "extract", os.path.relpath(self.logs))
        self.assertEqual((code, err), (0, ""))
        for name in ("stubs.json", "errors.json"):
            self.assertTrue((self.scn_dir / name).is_file(), name)
        self.assertEqual(os.listdir(self.cwd), [])

    def test_build_resolves_the_repro_file_against_the_scenario_dir(self):
        self.extract_and_build()
        for name in ("TRIAGE.md", "stubs.md", "SUMMARY.md"):
            self.assertTrue((self.scn_dir / name).is_file(), name)
        self.assertEqual(os.listdir(self.cwd), [])
        self.assertIn("- reproduced: not-run\n", (self.scn_dir / "TRIAGE.md").read_text())
        other = self.root / "other.json"
        other.write_text(json.dumps({"M1": {"reproduced": "no", "exit_code": 0,
                                            "first_line_run1": "a", "first_line_run2": "b"}}))
        self.assertEqual(self.run_cli(self.scn_dir, "build", other)[0], 0)
        self.assertIn("- reproduced: no\n", (self.scn_dir / "TRIAGE.md").read_text())

    def test_check_exits_zero_on_good_documents_and_one_on_a_problem(self):
        self.extract_and_build()
        self.assertEqual(self.run_cli(self.scn_dir, "check"), (0, "sections=1 nonclean=1 problems=0\n", ""))
        triage = self.scn_dir / "TRIAGE.md"
        triage.write_text(triage.read_text().replace("- owner:", "- own:"))
        code, out, _ = self.run_cli(self.scn_dir, "check")
        self.assertEqual(code, 1)
        self.assertIn("FAIL M1: missing owner line", out)

    def test_repro_resolves_the_output_file_against_the_scenario_dir(self):
        def fake_run(cmd, **kwargs):
            open(next(a for a in cmd if a.startswith("--smoke-log=")).split("=", 1)[1], "w").close()
            return mock.Mock(returncode=0)

        with mock.patch.object(repro.subprocess, "run", side_effect=fake_run) as run:
            code, out, _ = self.run_cli(self.scn_dir, "repro", "Engine", "<game>", self.logs, "out.json", "M1")
        self.assertEqual(code, 0)
        self.assertEqual(out, "M1 no 0\n")
        self.assertEqual(run.call_args.args[0][-1], "<game>")
        self.assertEqual(list(json.loads((self.scn_dir / "out.json").read_text())), ["M1"])
        self.assertEqual(os.listdir(self.cwd), [])

    def test_a_scenario_may_import_a_sibling_module(self):
        (self.scn_dir / "entry_sibling_tables.py").write_text("SIBLING_LABEL = 'From sibling'\n")
        self.write_scenario("from entry_sibling_tables import SIBLING_LABEL\n"
                            + self.SCENARIO.replace("LABEL = 'Entry'", "LABEL = SIBLING_LABEL"))
        self.addCleanup(sys.modules.pop, "entry_sibling_tables", None)
        self.addCleanup(setattr, sys, "path", list(sys.path))
        self.extract_and_build()
        self.assertTrue((self.scn_dir / "SUMMARY.md").read_text().startswith("# From sibling sweep summary\n"))

    def test_a_scenario_dir_must_define_exactly_one_scenario(self):
        scenario_file = str(self.scn_dir / "scenario.py")
        two = self.SCENARIO + "import dataclasses\nSECOND = dataclasses.replace(ENTRY, label='Second')\n"
        for label, source in (("none", "LABEL = 'Entry'\n"), ("two", two)):
            with self.subTest(label):
                self.write_scenario(source)
                code, out, err = self.run_cli(self.scn_dir, "check")
                self.assertNotEqual(code, 0)
                self.assertEqual(out, "")
                self.assertIn(scenario_file, err)
        os.remove(scenario_file)
        code, _, err = self.run_cli(self.scn_dir, "check")
        self.assertNotEqual(code, 0)
        self.assertIn(scenario_file, err)

    def test_usage_errors_exit_nonzero_with_the_usage(self):
        for argv in ([], [self.scn_dir], [self.scn_dir, "frobnicate"], [self.scn_dir, "extract"],
                     [self.scn_dir, "build"], [self.scn_dir, "check", "extra"],
                     [self.scn_dir, "repro", "exe", "game", self.logs, "out.json"]):
            with self.subTest(argv=argv):
                code, out, err = self.run_cli(*argv)
                self.assertNotEqual(code, 0)
                self.assertEqual(out, "")
                self.assertIn("usage:", err)

    def test_runs_as_a_file_under_isolated_mode(self):
        entry = SMOKE / "triage" / "__main__.py"
        done = subprocess.run([sys.executable, "-I", "-B", str(entry), str(SCENARIOS / "deusex"), "check"],
                              cwd=self.cwd, capture_output=True, text=True)
        self.assertEqual((done.returncode, done.stderr), (0, ""))
        self.assertIn("problems=0", done.stdout)


class TestReproStandsAlone(unittest.TestCase):
    def test_importing_repro_does_not_load_the_sweep(self):
        code = ("import sys; sys.path.insert(0, %r); from triage import repro; print('sweep' in sys.modules)"
                % str(SMOKE))
        done = subprocess.run([sys.executable, "-I", "-B", "-c", code], capture_output=True, text=True)
        self.assertEqual((done.returncode, done.stdout.strip()), (0, "False"), done.stderr)


class TestUtScenariosShareTables(unittest.TestCase):
    def test_botmatch_and_no_bots_hold_the_same_blocks(self):
        bots, plain = load_scenario("ut-bots"), load_scenario("ut")
        for block in ("hierarchy", "gameplay_roots", "bare_owners", "stub_owner_file"):
            with self.subTest(block):
                self.assertTrue(getattr(bots, block))
                self.assertIs(getattr(bots, block), getattr(plain, block))


class TestCommittedScenarios(unittest.TestCase):
    """The committed documents of each scenario are what the module builds from
    its committed sidecars, and its checker finds nothing to report."""

    def assert_regenerates(self, name):
        scn = load_scenario(name)
        data = SCENARIOS / name
        with tempfile.TemporaryDirectory() as out:
            build.build(scn, str(data), out, str(data / "repro.json"))
            for doc in ("TRIAGE.md", "stubs.md", "SUMMARY.md"):
                self.assertTrue(filecmp.cmp(os.path.join(out, doc), data / doc, shallow=False),
                                "%s/%s" % (name, doc))
            _, _, problems = check.check(scn, str(data), out)
            with open(os.path.join(out, "SUMMARY.md")) as f:
                summary = f.read()
        self.assertEqual(problems, [])
        # An owner keyed by a crash signature must use the module's spelling, or it never applies.
        for key in scn.error_owner:
            if key.startswith("crash:"):
                self.assertIn("| %s |" % key, summary)

    def test_deusex(self):
        self.assert_regenerates("deusex")

    def test_ut_bots(self):
        self.assert_regenerates("ut-bots")

    def test_unreal(self):
        self.assert_regenerates("unreal")

    def test_ut(self):
        self.assert_regenerates("ut")


class TestScenarioDefaults(unittest.TestCase):
    def test_scenario_has_no_defaults_naming_a_game(self):
        for f in dataclasses.fields(Scenario):
            if f.default is not dataclasses.MISSING:
                self.assertNotIn("Deus", repr(f.default))


if __name__ == "__main__":
    unittest.main()
