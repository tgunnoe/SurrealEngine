"""Generate TRIAGE.md, stubs.md and SUMMARY.md from the committed sidecars.

usage: python3 -I build_triage.py <repro.json>

Run from this directory. Reads sweep.json, stubs.json, errors.json and the
re-run results in repro.json (also committed).
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.dirname(os.path.dirname(HERE)))
from scenario import DEUSEX  # noqa: E402

from triage import build  # noqa: E402

if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    build.build(DEUSEX, ".", ".", sys.argv[1])
