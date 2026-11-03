"""Run the triage module on a scenario directory, as a file or as a module:

    python3 -I -B Tools/smoke/triage/__main__.py <scenario dir> <command> ...

Isolated mode puts no directory on sys.path, so the file adds the one that
holds the triage package. cli.py documents the commands.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from triage import cli  # noqa: E402

if __name__ == "__main__":
    sys.exit(cli.main(sys.argv[1:]))
