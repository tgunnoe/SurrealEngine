import contextlib
import io
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent))
import sweep


def make_info(**overrides):
    info = {"summary": True, "script_errors": 0, "missing_natives": 0,
            "exceptions": 0, "script_warnings": 0, "fatal_signal": False,
            "maps_loaded": ["Bluff"], "fps_avg": 60.0}
    info.update(overrides)
    return info


def verdict(info=None, exit_code=0, timed_out=False, requested_map="Bluff"):
    return sweep.classify(info or make_info(), exit_code, timed_out, requested_map)


class ClassifyTests(unittest.TestCase):
    def test_pass(self):
        self.assertEqual(verdict(), "pass")

    def test_idle_when_requested_map_not_loaded(self):
        self.assertEqual(verdict(make_info(maps_loaded=["Other"])), "idle")
        self.assertEqual(verdict(make_info(maps_loaded=[])), "idle")

    def test_idle_when_fps_missing(self):
        self.assertEqual(verdict(make_info(fps_avg=None)), "idle")

    def test_map_match_is_exact(self):
        self.assertEqual(verdict(make_info(maps_loaded=["bluff"])), "idle")

    def test_fail_on_errors(self):
        self.assertEqual(verdict(make_info(script_errors=1)), "fail")
        self.assertEqual(verdict(make_info(missing_natives=1)), "fail")
        self.assertEqual(verdict(make_info(exceptions=1)), "fail")

    def test_fail_on_nonzero_exit_or_missing_summary(self):
        self.assertEqual(verdict(exit_code=1), "fail")
        self.assertEqual(verdict(make_info(summary=False)), "fail")

    def test_timeout_beats_everything(self):
        info = make_info(fatal_signal=True, script_errors=3, maps_loaded=[])
        self.assertEqual(verdict(info, exit_code=-9, timed_out=True), "timeout")

    def test_crash_beats_idle_and_fail(self):
        info = make_info(fatal_signal=True, script_errors=3, maps_loaded=[])
        self.assertEqual(verdict(info, exit_code=-11), "crash")

    def test_warnings_leave_verdict_alone(self):
        self.assertEqual(verdict(make_info(script_warnings=500)), "pass")
        self.assertEqual(verdict(make_info(script_warnings=5, script_errors=1)), "fail")


def parse(text):
    with tempfile.TemporaryDirectory() as d:
        log = Path(d) / "a.log"
        log.write_text(text)
        return sweep.parse_log(log)[0]


WAITING = "STATE 0.0 PlayerWaiting at (1.0, 2.0, 3.0)\n"
WALKING_AT_START = "STATE 0.0 PlayerWalking at (1.0, 2.0, 3.0)\n"


class ParseLogTests(unittest.TestCase):
    def test_counts_script_warnings(self):
        info = parse("GAME x 1\nMAP Bluff\nSCRIPT_WARNING Accessed None in A.B\n"
                     "SCRIPT_WARNING Accessed None when calling C\nLOGIN\n"
                     "SUMMARY script_errors=0\n")
        self.assertEqual(info["script_warnings"], 2)
        self.assertEqual(info["maps_loaded"], ["Bluff"])

    def test_login_follows_the_login_line(self):
        self.assertTrue(parse("GAME x 1\nMAP A\nLOGIN\n")["login"])
        self.assertFalse(parse("GAME x 1\nMAP A\nSCRIPT_ERROR x\n")["login"])

    def test_the_log_is_read_as_utf8_like_the_rerun_comparison(self):
        # The locale default decodes UTF-8 on most hosts, so no byte-level
        # input separates the readers there; the call itself is what is pinned.
        with mock.patch.object(Path, "read_text", autospec=True, return_value="") as read:
            sweep.parse_log(Path("a.log"))
        read.assert_called_once()
        self.assertEqual(read.call_args.kwargs, {"encoding": "utf-8", "errors": "replace"})

    def test_unreadable_log_has_no_login_and_no_match(self):
        info, _ = sweep.parse_log(Path(tempfile.gettempdir()) / "no-such-sweep-log.log")
        self.assertFalse(info["login"])
        self.assertFalse(info["match_started"])


class MatchStartedTests(unittest.TestCase):
    """The match started when the player was walking at the fire press or
    walked afterwards; only a console command that ran counts as the press."""

    def started(self, body):
        return parse("GAME x 1\nMAP A\nLOGIN\n" + body)["match_started"]

    def test_walking_when_the_press_ran(self):
        self.assertTrue(self.started(WALKING_AT_START + "EXEC 5 Fire\n"))

    def test_walking_after_the_press(self):
        self.assertTrue(self.started(WAITING + "EXEC 5 Fire\nSTATE 6.0 PlayerWalking\n"))

    def test_last_state_before_the_press_is_the_one_in_effect(self):
        self.assertFalse(self.started(WALKING_AT_START + "STATE 3.0 PlayerWaiting\nEXEC 5 Fire\n"))

    def test_failed_press_does_not_count(self):
        self.assertFalse(self.started(WALKING_AT_START + "EXEC_FAILED 5 Fire\nSTATE 6.0 PlayerWalking\n"))

    def test_skipped_press_does_not_count(self):
        self.assertFalse(self.started(WAITING + "EXEC_SKIPPED 5 Fire\nSTATE 9.0 PlayerWalking\n"))

    def test_no_press_means_no_match(self):
        self.assertFalse(self.started(WALKING_AT_START))

    def test_press_without_walking_means_no_match(self):
        self.assertFalse(self.started(WAITING + "EXEC 5 Fire\nSTATE 9.0 PlayerSpectating\n"))


class SharedMatchRuleTests(unittest.TestCase):
    """The start rule lives in the triage package, so a re-run and a sweep row
    cannot disagree on what a start is."""

    def test_parse_log_takes_the_verdict_from_the_shared_rule(self):
        from triage import matchrule
        self.assertIs(sweep.match_started, matchrule.match_started)
        with mock.patch.object(sweep, "match_started", return_value=True) as rule:
            self.assertIs(parse("GAME x 1\nMAP A\nLOGIN\n")["match_started"], True)
        rule.assert_called_once()

    def test_runs_as_a_file_under_isolated_mode(self):
        # Isolated mode puts no directory on sys.path; the script finds the package itself.
        done = subprocess.run([sys.executable, "-I", "-B", sweep.__file__, "--help"],
                              capture_output=True, text=True)
        self.assertEqual((done.returncode, done.stderr), (0, ""))
        self.assertIn("usage:", done.stdout)


FAKE_ENGINE = """#!{python}
import json
import sys

args = sys.argv[1:]
log = next(a for a in args if a.startswith("--smoke-log=")).split("=", 1)[1]
with open(log + ".argv", "w") as f:
    json.dump(args, f)
with open(log, "w") as f:
    f.write("GAME x 1\\nMAP A\\nLOGIN\\n" + {state!r} + "EXEC 5 Fire\\nSTATE 6.0 PlayerWalking\\n"
            "FPS 30.0\\nSUMMARY script_errors=0\\n")
"""


class SweepRunTests(unittest.TestCase):
    """Whole sweeps against a stand-in engine that records its command line."""

    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.root = Path(tmp.name)
        self.game = self.root / "Game"
        (self.game / "Maps").mkdir(parents=True)
        (self.game / "Maps" / "A.unr").write_bytes(b"")
        self.exe = self.root / "engine"
        self.exe.write_text(FAKE_ENGINE.format(python=sys.executable, state=WAITING))
        self.exe.chmod(0o755)
        self.logdir = self.root / "logs"
        self.out = self.root / "report.json"

    def sweep(self, *flags):
        argv = ["sweep.py", "--game", str(self.game), "--seconds", "2", "--out", str(self.out),
                "--exe", str(self.exe), "--logdir", str(self.logdir), *flags]
        with mock.patch.object(sys, "argv", argv), contextlib.redirect_stdout(io.StringIO()):
            sweep.main()
        report = json.loads(self.out.read_text())
        argv_seen = json.loads((self.logdir / "A.log.argv").read_text())
        return report, argv_seen

    def test_options_reach_the_command_line_and_the_report(self):
        report, args = self.sweep("--url-options", "?NumBots=4", "--exec", "5:Fire")
        self.assertIn("--url=A?NumBots=4", args)
        self.assertIn("--exec=5:Fire", args)
        self.assertEqual(args[-1], str(self.game))
        self.assertEqual(report["url_options"], "?NumBots=4")
        self.assertEqual(report["exec"], "5:Fire")
        self.assertEqual(report["game"], str(self.game))
        row = report["maps"][0]
        self.assertEqual((row["login"], row["match_started"], row["verdict"]), (True, True, "pass"))

    def test_without_the_options_the_command_is_unchanged(self):
        report, args = self.sweep()
        self.assertIn("--url=A", args)
        self.assertEqual([a for a in args if a.startswith("--exec")], [])
        self.assertIsNone(report["url_options"])
        self.assertIsNone(report["exec"])
        self.assertEqual(report["maps"][0]["log"], str(self.logdir / "A.log"))

    def test_portable_report_names_no_host_path(self):
        report, _ = self.sweep("--portable")
        self.assertEqual(report["game"], "<game folder>")
        row = report["maps"][0]
        self.assertEqual(row["log"], "A.log")
        self.assertIsNone(row["screenshot"])

    def add_maps(self, folder, *stems):
        path = self.game / "Maps" / folder
        path.mkdir(parents=True, exist_ok=True)
        for stem in stems:
            (path / f"{stem}.unr").write_bytes(b"")

    def rows(self, report):
        return [row["map"] for row in report["maps"]]

    def refusal(self, *flags):
        """The exit text of a sweep that must stop before it starts any engine."""
        with self.assertRaises(SystemExit) as cm:
            self.sweep(*flags)
        self.assertEqual(list(self.logdir.glob("*.argv")), [])
        return str(cm.exception.code)

    def test_map_subdir_maps_run_after_the_top_level_by_bare_name(self):
        self.add_maps("UPak", "0B")
        report, _ = self.sweep("--map-subdir", "UPak")
        self.assertEqual(self.rows(report), ["A", "0B"])
        args = json.loads((self.logdir / "0B.log.argv").read_text())
        self.assertIn("--url=0B", args)
        self.assertIn(f"--smoke-log={self.logdir / '0B.log'}", args)
        self.assertEqual(report["map_subdirs"], ["UPak"])

    def test_map_subdir_absent_lists_the_top_level_only(self):
        self.add_maps("UPak", "0B")
        report, _ = self.sweep()
        self.assertEqual(self.rows(report), ["A"])
        self.assertIsNone(report["map_subdirs"])

    def test_map_subdir_folders_run_in_the_order_given(self):
        self.add_maps("Y", "0C")
        self.add_maps("X", "0D")
        report, _ = self.sweep("--map-subdir", "Y", "--map-subdir", "X")
        self.assertEqual(self.rows(report), ["A", "0C", "0D"])
        self.assertEqual(report["map_subdirs"], ["Y", "X"])

    def test_map_subdir_stem_in_two_folders_is_refused(self):
        self.add_maps("UPak", "A")
        text = self.refusal("--map-subdir", "UPak")
        self.assertIn("A", text)
        self.assertIn("more than one folder", text)

    def test_map_subdir_names_differing_by_case_in_two_subfolders_are_refused(self):
        # The engine compares map names without regard to ASCII case, so q and Q are one map.
        self.add_maps("X", "q")
        self.add_maps("Y", "Q")
        text = self.refusal("--map-subdir", "X", "--map-subdir", "Y")
        self.assertIn("Q", text)
        self.assertIn("more than one folder", text)

    def test_map_subdir_missing_folder_is_refused(self):
        text = self.refusal("--map-subdir", "Nope")
        self.assertIn("Maps/Nope", text)

    def test_map_subdir_maps_count_toward_the_limit_after_the_top_level(self):
        self.add_maps("UPak", "0B")
        report, _ = self.sweep("--map-subdir", "UPak", "--limit", "1")
        self.assertEqual(self.rows(report), ["A"])
        self.assertFalse((self.logdir / "0B.log.argv").exists())

    def test_a_repeated_subfolder_is_listed_once_in_first_mention_order(self):
        self.add_maps("Y", "0C")
        self.add_maps("X", "0D")
        report, _ = self.sweep("--map-subdir", "Y", "--map-subdir", "X", "--map-subdir", "Y")
        self.assertEqual(self.rows(report), ["A", "0C", "0D"])
        self.assertEqual(report["map_subdirs"], ["Y", "X"])

    def test_a_subfolder_name_must_be_a_bare_folder_name(self):
        # A folder beside Maps with a map in it: "../Outside" would list it if it were let through.
        (self.game / "Outside").mkdir()
        (self.game / "Outside" / "Z.unr").write_bytes(b"")
        for name in ["", ".", "..", "UPak/x", "../Outside", "a\\b"]:
            with self.subTest(name=name):
                self.assertIn("takes a folder name under Maps", self.refusal("--map-subdir", name))

    def argv_of(self, name):
        return json.loads((self.logdir / f"{name}.log.argv").read_text())

    def sweep_by_prefix(self):
        """One sweep over assault, deathmatch and dashless maps with AS mapped to Assault."""
        for stem in ["AS-One", "as-Two", "AS-Hi-Speed", "DM-Two", "AS"]:
            (self.game / "Maps" / f"{stem}.unr").write_bytes(b"")
        report, _ = self.sweep("--game-by-prefix", "AS=Botpack.Assault", "--url-options", "?NumBots=4")
        return report

    def test_game_by_prefix_names_the_game_right_after_the_map_name(self):
        report = self.sweep_by_prefix()
        # The match folds ASCII case, and the game text comes before the url options.
        self.assertIn("--url=AS-One?game=Botpack.Assault?NumBots=4", self.argv_of("AS-One"))
        self.assertIn("--url=as-Two?game=Botpack.Assault?NumBots=4", self.argv_of("as-Two"))
        # The prefix ends at the first dash.
        self.assertIn("--url=AS-Hi-Speed?game=Botpack.Assault?NumBots=4", self.argv_of("AS-Hi-Speed"))
        self.assertEqual(report["game_by_prefix"], {"AS": "Botpack.Assault"})

    def test_game_by_prefix_leaves_other_prefixes_and_dashless_names_alone(self):
        self.sweep_by_prefix()
        # A name with no dash has no prefix to match, not even one spelled like the mapped
        # prefix.
        self.assertIn("--url=DM-Two?NumBots=4", self.argv_of("DM-Two"))
        self.assertIn("--url=A?NumBots=4", self.argv_of("A"))
        self.assertIn("--url=AS?NumBots=4", self.argv_of("AS"))
        for name in ["DM-Two", "A", "AS"]:
            self.assertEqual([a for a in self.argv_of(name) if "game=" in a], [])

    def test_without_game_by_prefix_the_command_and_report_are_unchanged(self):
        report, args = self.sweep()
        self.assertIn("--url=A", args)
        self.assertIsNone(report["game_by_prefix"])

    def test_game_by_prefix_malformed_or_repeated_entries_are_refused(self):
        bad = "--game-by-prefix: bad entry '%s' (expected PREFIX=Package.Class)"
        self.assertIn(bad % "AS", self.refusal("--game-by-prefix", "AS"))
        text = self.refusal("--game-by-prefix", "AS=Botpack.Assault",
                            "--game-by-prefix", "as=Botpack.CTFGame")
        self.assertIn("--game-by-prefix: prefix 'as' given twice", text)
        # The rest of the entry form: a prefix that is a bare word and a class that is
        # Package.Class, with nothing in it that could add a url option.
        for entry in ["=Botpack.Assault", "AS-=Botpack.Assault", "A S=Botpack.Assault",
                      "AS/x=Botpack.Assault", "AS=", "AS=Assault", "AS=Botpack.Assault.Extra",
                      "AS=Botpack.Assault?NumBots=4", "AS=Botpack.Assault&x", ".=Botpack.Assault",
                      "AS=Botpack.Assault\n"]:
            with self.subTest(entry=entry):
                self.assertIn(bad % entry, self.refusal("--game-by-prefix", entry))

    def sweep_ctf(self, *flags):
        """One sweep over CTF-Tutorial and CTF-Face with CTF mapped to Botpack.CTFGame."""
        for stem in ["CTF-Tutorial", "CTF-Face"]:
            (self.game / "Maps" / f"{stem}.unr").write_bytes(b"")
        report, _ = self.sweep("--game-by-prefix", "CTF=Botpack.CTFGame", *flags)
        return report

    def test_game_by_prefix_except_keeps_the_named_map_free_of_the_game(self):
        report = self.sweep_ctf("--game-by-prefix-except", "CTF-Tutorial")
        # An argv entry is compared whole, so this also pins that no ?game= text follows.
        self.assertIn("--url=CTF-Tutorial", self.argv_of("CTF-Tutorial"))
        self.assertIn("--url=CTF-Face?game=Botpack.CTFGame", self.argv_of("CTF-Face"))
        self.assertEqual(report["game_by_prefix_except"], ["CTF-Tutorial"])

    def test_game_by_prefix_except_folds_ascii_case(self):
        self.sweep_ctf("--game-by-prefix-except", "ctf-tutorial")
        self.assertIn("--url=CTF-Tutorial", self.argv_of("CTF-Tutorial"))
        # Only the named map is exempt, not every map of the prefix.
        self.assertIn("--url=CTF-Face?game=Botpack.CTFGame", self.argv_of("CTF-Face"))

    def test_without_game_by_prefix_except_the_report_key_is_null(self):
        report = self.sweep_ctf()
        self.assertIsNone(report["game_by_prefix_except"])
        self.assertIn("--url=CTF-Tutorial?game=Botpack.CTFGame", self.argv_of("CTF-Tutorial"))

    def test_game_by_prefix_except_bad_or_repeated_names_are_refused(self):
        bad = "--game-by-prefix-except: bad name '%s'"
        self.assertIn(bad % "CTF/Tutorial", self.refusal("--game-by-prefix-except", "CTF/Tutorial"))
        text = self.refusal("--game-by-prefix-except", "CTF-Tutorial",
                            "--game-by-prefix-except", "ctf-tutorial")
        self.assertIn("--game-by-prefix-except: name 'ctf-tutorial' given twice", text)
        # The rest of the name form: a bare file stem, with no separator, no blank and no
        # stand-in for the current or parent folder.
        for name in ["", ".", "..", "CTF\\Tutorial", "CTF Tutorial", "CTF\tTutorial", "CTF-Tutorial\n"]:
            with self.subTest(name=name):
                self.assertIn(bad % name, self.refusal("--game-by-prefix-except", name))


if __name__ == "__main__":
    unittest.main()
