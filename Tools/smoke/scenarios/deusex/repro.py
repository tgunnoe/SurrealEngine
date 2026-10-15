"""Re-run maps individually and compare against the committed sidecars.

usage: python3 -I repro.py <exe> <game folder> <logdir> <out.json> <map>...

Needs the run environment from Tools/smoke/README.md. Logs go to
/tmp/repro-deusex/<map>.log and are not meant to be committed.
"""
import json
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import extract  # noqa: E402

HEX = re.compile(r"0x[0-9a-fA-F]+")


def first_failing(path):
    stubs, errors, _ = extract.parse(path)
    if errors:
        return "%s %s" % (errors[0]["kind"], HEX.sub("0x?", errors[0]["text"]))
    return None


def gameplay_stubs(path):
    stubs, _, _ = extract.parse(path)
    return sorted(s for s in stubs if extract.classify(s.split(".", 1)[0])[0] != "non-gameplay")


def main(exe, game, logdir, out, maps):
    os.makedirs("/tmp/repro-deusex", exist_ok=True)
    res = {}
    for m in maps:
        log = "/tmp/repro-deusex/%s.log" % m
        if os.path.exists(log):
            os.remove(log)
        cmd = [exe, "--autostart", "--exit-after=60", "--smoke-log=" + log, "--url=" + m, game]
        try:
            code = subprocess.run(cmd, timeout=105, stdout=subprocess.DEVNULL,
                                  stderr=subprocess.DEVNULL).returncode
        except subprocess.TimeoutExpired:
            code = "timeout"
        old = os.path.join(logdir, m + ".log")
        a, b = first_failing(old), first_failing(log)
        if a is None and b is None:
            same = gameplay_stubs(old) == gameplay_stubs(log)
            verdict = "yes" if same else "flaky"
            a = "stubs:" + ",".join(gameplay_stubs(old))
            b = "stubs:" + ",".join(gameplay_stubs(log))
        elif a is None or b is None:
            verdict = "no" if b is None else "flaky"
        else:
            verdict = "yes" if a == b else "flaky"
        res[m] = {"reproduced": verdict, "exit_code": code, "first_line_run1": a, "first_line_run2": b}
        print(m, res[m]["reproduced"], code, flush=True)
        json.dump(res, open(out, "w"), indent=1, sort_keys=True)


if __name__ == "__main__":
    if len(sys.argv) < 6:
        sys.exit(__doc__)
    main(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5:])
