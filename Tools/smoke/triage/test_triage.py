import dataclasses
import filecmp
import importlib.util
import json
import os
import sys
import tempfile
import unittest
from pathlib import Path

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
    sweep = [row(n) for n in names] + [row("C1"), row("Cased", ["cased"])]
    stubs = {n: [stub()] for n in names}
    stubs["C1"] = []
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


class TestDeusExByteIdentity(unittest.TestCase):
    def test_regenerates_committed_documents(self):
        scn = load_deusex()
        with tempfile.TemporaryDirectory() as out:
            build.build(scn, str(DEUSEX_DIR), out, str(DEUSEX_DIR / "repro.json"))
            for name in ("TRIAGE.md", "stubs.md", "SUMMARY.md"):
                self.assertTrue(filecmp.cmp(os.path.join(out, name), DEUSEX_DIR / name, shallow=False), name)
            sections, nonclean, problems = check.check(scn, str(DEUSEX_DIR), out)
        self.assertEqual((sections, nonclean, problems), (88, 88, []))

    def test_scenario_has_no_defaults_naming_a_game(self):
        for f in dataclasses.fields(Scenario):
            if f.default is not dataclasses.MISSING:
                self.assertNotIn("Deus", repr(f.default))


if __name__ == "__main__":
    unittest.main()
