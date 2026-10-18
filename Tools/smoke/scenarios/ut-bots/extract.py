"""Extract per-map STUB and error sidecars from a directory of smoke logs.

usage: python3 -I extract.py <logdir> <outdir>

Writes <outdir>/stubs.json and <outdir>/errors.json. See Tools/smoke/README.md
for the log grammar.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.dirname(os.path.dirname(HERE)))
from scenario import UTBOTS  # noqa: E402

from triage import extract  # noqa: E402

if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    extract.extract(UTBOTS, sys.argv[1], sys.argv[2])
