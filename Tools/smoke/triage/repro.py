"""Re-run maps individually and compare against earlier logs.

Needs the run environment from Tools/smoke/README.md. Logs go to
Scenario.repro_logdir and are not meant to be committed.
"""
import json
import os
import subprocess

import sweep

from . import extract


def _first_failing(path):
    _, errors, _, _ = extract.parse(path)
    if errors:
        return "%s %s" % (errors[0]["kind"], extract.HEX.sub("0x?", errors[0]["text"]))
    return None


def _gameplay_stubs(scn, path):
    stubs, _, _, _ = extract.parse(path)
    return sorted(s for s in stubs
                  if extract.classify(scn, extract.owner_of(scn, s))[0] != "non-gameplay")


def _match_started(path):
    # The sweep's own rule: a re-run and a sweep row cannot disagree on what a start is.
    return sweep.parse_log(path)[0]["match_started"]


def log_path(scn, name):
    return os.path.join(scn.repro_logdir, name + ".log")


def engine_args(scn, exe, name):
    """The engine command for one map up to the game folder: the options the
    sweep used, so the re-run is the same run. The documents show it too."""
    args = [exe, "--autostart", "--exit-after=%d" % scn.exit_after_seconds,
            "--smoke-log=" + log_path(scn, name),
            "--url=" + name + "".join(scn.url_options)]
    if scn.exec:
        args.append("--exec=" + scn.exec)
    return args


def compare(scn, old_log, new_log):
    """Return (verdict, first_line_run1, first_line_run2).

    Runs agree when their first failing line matches after masking addresses;
    with no failing line in either, when their gameplay stub sets match. When
    the scenario requires a match start, the runs must also agree on whether
    the match started, and a non-start counts as a failure: the verdict is
    "no" when the first run failed (a failing line or a non-start) and the
    second did not, "flaky" when the runs disagree otherwise. Each line then
    ends with "; match_started:<true|false>".
    """
    a, b = _first_failing(old_log), _first_failing(new_log)
    if a is None and b is None:
        old, new = _gameplay_stubs(scn, old_log), _gameplay_stubs(scn, new_log)
        agree, line1, line2 = old == new, "stubs:" + ",".join(old), "stubs:" + ",".join(new)
    else:
        agree, line1, line2 = a == b, a, b
    failed1, failed2 = a is not None, b is not None
    if scn.require_match_start:
        started1, started2 = _match_started(old_log), _match_started(new_log)
        agree = agree and started1 == started2
        failed1, failed2 = failed1 or not started1, failed2 or not started2
        line1 = "%s; match_started:%s" % (line1 or "no failing line", str(started1).lower())
        line2 = "%s; match_started:%s" % (line2 or "no failing line", str(started2).lower())
    if agree:
        return "yes", line1, line2
    return ("no" if failed1 and not failed2 else "flaky"), line1, line2


def run(scn, exe, game, logdir, out, maps):
    os.makedirs(scn.repro_logdir, exist_ok=True)
    res = {}
    for m in maps:
        log = log_path(scn, m)
        if os.path.exists(log):
            os.remove(log)
        try:
            code = subprocess.run([*engine_args(scn, exe, m), game], timeout=scn.repro_timeout_seconds, stdout=subprocess.DEVNULL,
                                  stderr=subprocess.DEVNULL).returncode
        except subprocess.TimeoutExpired:
            code = "timeout"
        verdict, a, b = compare(scn, os.path.join(logdir, m + ".log"), log)
        res[m] = {"reproduced": verdict, "exit_code": code, "first_line_run1": a, "first_line_run2": b}
        print(m, verdict, code, flush=True)
        with open(out, "w") as f:
            json.dump(res, f, indent=1, sort_keys=True)
