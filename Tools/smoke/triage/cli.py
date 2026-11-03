"""The command line of the triage module.

    <scenario dir> extract <logdir>
    <scenario dir> repro <exe> <game folder> <logdir> <out.json> <map>...
    <scenario dir> build <repro.json>
    <scenario dir> check

A scenario directory holds a scenario.py that defines exactly one
triage.scenario.Scenario, and the sweep's sidecars and documents. Sidecars and
documents are read from and written to the scenario directory.
"""
import importlib.util
import os
import re
import sys

from . import build, check, extract, repro
from .scenario import Scenario

SCENARIOS = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "scenarios")

USAGE = """usage: python3 -I -B Tools/smoke/triage/__main__.py <scenario dir> <command> ...
  extract <logdir>
  repro <exe> <game folder> <logdir> <out.json> <map>...
  build <repro.json>
  check
<repro.json> and <out.json> are relative to the scenario directory unless
absolute; <logdir>, <exe> and <game folder> are relative to the working
directory."""


class ScenarioError(Exception):
    """A scenario directory that does not define exactly one Scenario."""


def load_scenario(directory):
    """The Scenario defined by <directory>/scenario.py, loaded by path.

    The scenario directory and Tools/smoke/scenarios go on sys.path first, so a
    scenario can import a module that sits beside it or in the scenarios
    directory.
    """
    directory = os.path.abspath(directory)
    path = os.path.join(directory, "scenario.py")
    if not os.path.isfile(path):
        raise ScenarioError("%s: no such file; a scenario directory holds a scenario.py" % path)
    for entry in (SCENARIOS, directory):
        if entry not in sys.path:
            sys.path.insert(0, entry)
    spec = importlib.util.spec_from_file_location(
        "triage_scenario_" + re.sub(r"\W", "_", os.path.basename(directory)), path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    names = sorted(k for k, v in vars(module).items() if isinstance(v, Scenario))
    if len(names) != 1:
        raise ScenarioError("%s: expected exactly one Scenario, found %s"
                            % (path, ", ".join(names) if names else "none"))
    return getattr(module, names[0])


def usage():
    print(USAGE, file=sys.stderr)
    return 2


def main(argv):
    """Run one command; return the process exit status."""
    if len(argv) < 2:
        return usage()
    directory, command, args = os.path.abspath(argv[0]), argv[1], argv[2:]
    arity_ok = {"extract": len(args) == 1, "repro": len(args) >= 5,
                "build": len(args) == 1, "check": not args}.get(command)
    if not arity_ok:
        return usage()
    try:
        scn = load_scenario(directory)
    except ScenarioError as e:
        print("triage: %s" % e, file=sys.stderr)
        return 1
    if command == "extract":
        extract.extract(scn, args[0], directory)
    elif command == "repro":
        exe, game, logdir, out, *maps = args
        repro.run(scn, exe, game, logdir, os.path.join(directory, out), maps)
    elif command == "build":
        build.build(scn, directory, directory, os.path.join(directory, args[0]))
    else:
        sections, nonclean, problems = check.check(scn, directory, directory)
        print(check.format_result(sections, nonclean, problems))
        return 1 if problems else 0
    return 0
