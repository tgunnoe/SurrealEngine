"""Re-run maps individually and compare against the committed sidecars.

usage: python3 -I repro.py <exe> <game folder> <logdir> <out.json> <map>...

Needs the run environment from Tools/smoke/README.md. Logs go to
/tmp/repro-unreal/<map>.log and are not meant to be committed.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.dirname(os.path.dirname(HERE)))
from scenario import UNREAL  # noqa: E402

from triage import repro  # noqa: E402

if __name__ == "__main__":
    if len(sys.argv) < 6:
        sys.exit(__doc__)
    repro.run(UNREAL, sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5:])
