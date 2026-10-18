import contextlib
import io
import json
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


if __name__ == "__main__":
    unittest.main()
