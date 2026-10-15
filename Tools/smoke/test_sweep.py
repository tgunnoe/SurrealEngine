import sys
import unittest
from pathlib import Path

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


class ParseLogTests(unittest.TestCase):
    def test_counts_script_warnings(self):
        import tempfile
        with tempfile.TemporaryDirectory() as d:
            log = Path(d) / "a.log"
            log.write_text("GAME x 1\nMAP Bluff\nSCRIPT_WARNING Accessed None in A.B\n"
                           "SCRIPT_WARNING Accessed None when calling C\nLOGIN\n"
                           "SUMMARY script_errors=0\n")
            info, _ = sweep.parse_log(log)
        self.assertEqual(info["script_warnings"], 2)
        self.assertEqual(info["maps_loaded"], ["Bluff"])


if __name__ == "__main__":
    unittest.main()
