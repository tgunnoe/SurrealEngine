"""Check TRIAGE.md, stubs.md and SUMMARY.md against the committed data.

usage: python3 -I check_triage.py   (any directory; reads files beside it)
Exit 0 only when every check holds.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.dirname(os.path.dirname(HERE)))
from scenario import UTBOTS  # noqa: E402

from triage import check  # noqa: E402

if __name__ == "__main__":
    sections, nonclean, problems = check.check(UTBOTS, HERE, HERE)
    print(check.format_result(sections, nonclean, problems))
    sys.exit(1 if problems else 0)
