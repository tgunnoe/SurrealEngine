#!/usr/bin/env python3
"""Per-run pins for the smoke harness (stub)."""
from pathlib import Path
from typing import NamedTuple


class Pins(NamedTuple):
    config_dir: str | None
    init_options: list
    level_info: list


def class_option(player_class):
    return ""


def pinned_url(map_name, game_text="", player_class=None, url_options=""):
    return map_name


def game_key(exe, version, root_folder):
    return f"{exe}-{version}-root"


def pin_bots(ini_text, count):
    return ini_text


def prepare(game, kind, config_root, bots=None, user_dir=None, host_root=None):
    return Path(config_root), None


def read_pins(lines):
    return Pins(None, [], [])


def check_pins(lines, expected_config, player_class=None):
    return True, []
