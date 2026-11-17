#!/usr/bin/env python3
"""The twelve mover and zone pins and their judges (stub)."""
from dataclasses import dataclass
from typing import NamedTuple

VOID = "VOID"


@dataclass(frozen=True)
class Pin:
    id: str
    game: str
    url: str
    exec: str
    trace: str
    exit_after: int
    player_class: str
    judge: str
    params: dict

    def full_url(self):
        return ""

    def args(self, smoke_log="<smoke log>"):
        return []


class Run(NamedTuple):
    smoke: list
    answers: list
    traces: list
    places: list


def _pin(pin_id, params=None):
    return Pin(pin_id, "", "", "", "", 0, "", "", params or {})


PINS = {p.id: p for p in [
    _pin("mv-proj-ug"), _pin("mv-proj-ut"),
    _pin("mv-slide-ug", {"t0": 11.5, "t1": 13.0}), _pin("mv-slide-dx", {"t0": 5.75, "t1": 9.0}),
    _pin("mv-enc-ug"), _pin("mv-enc-ut"), _pin("mv-enc-dx"),
    _pin("zn-w1"), _pin("zn-w2"), _pin("zn-ctf1"), _pin("zn-ctf2"), _pin("zn-ctf3"),
]}


def read_run(smoke_text, engine_text):
    return Run([], [], [], [])


def player_name(run, class_word):
    return None


def exec_items(exec_text):
    return []


def judge(pin, smoke_text, engine_text):
    return VOID, []
