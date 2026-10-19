#!/usr/bin/env python3
"""Run one game's save, quit and load chain across three maps (stubs).

Every function below is deliberately wrong, so that the tests in
test_saveload.py fail on their assertions before the real code exists.
"""

STEP_NAMES = ()
STEP_WHERES = ()
DEFAULT_SECONDS = 0
CHAINS = {}


def parse_engine_log(lines):
    window = {"where": "after-state", "map": None,
              "items": [{"package": "Stub", "instance": "Stub0", "class": "Stub"}],
              "calls": [], "weapon": None, "offered": [], "travel_skipped": False}
    return {"windows": [window], "loaded_slot": None, "loaded_map": None,
            "saved_to": None, "summoned": [], "skip_lines": []}


def parse_smoke_exec(lines):
    return {"failed": ["stub"], "skipped": ["stub"]}


def compare(before, after):
    return {"kept": [], "lost": [i["class"] for i in before], "gained": []}


def call_status(calls, function):
    return "stub"


def build_exec(plan):
    return ""


def step_plan(chain, index):
    return []


def step_verdict(info, exit_code, timed_out, expected_map):
    return "stub"


def build_hops(steps):
    return []


def verdict_sentence(report):
    return ""


def render_markdown(report):
    return ""


def step_problem(step, index):
    return "stub"


def assemble_checks(parsed, index, chain):
    return {"checks": [], "offered": [], "travel_skipped": True}
