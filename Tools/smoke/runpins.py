#!/usr/bin/env python3
"""Per-run pins for the smoke harness: the player class, the bot count and the evidence.

A run judged against a value recorded on another date pins its environment itself and never
edits a game folder:

  class      the URL's ?Class= option, which the engine reads before the system ini's [URL] Class;
  bot count  an SE-UnrealTournament.ini in the run's own config folder, a copy of the install's
             ini with MinPlayers and InitialBots set (Unreal Tournament reads no URL option for it);
  evidence   the engine log of the run names the config folder in use and the class on its
             InitGame line, and `check` reads both back.

Standard library only. Command line:
  runpins.py prepare --game DIR --kind unreal|ut|deusex --config-root DIR [--bots N] [--user-dir DIR]
  runpins.py check --engine-log FILE --config-root DIR --kind KIND [--game DIR] [--user-dir DIR]
                   [--class Package.Class] [--bots N]
"""
import argparse
import os
import re
import shutil
import sys
from pathlib import Path
from typing import NamedTuple

PLAYER_CLASS = "UPak.UPakFemaleOne"
# The (executable name, version) the engine puts at the front of a game's folder key.
KINDS = {"unreal": ("Unreal", "226b"), "ut": ("UnrealTournament", "436"), "deusex": ("DeusEx", "1112fm")}
# The engine's bound on the install folder's part of the key (UserDirs.cpp).
MAX_KEY_LEAF = 32
APP_DIR = "SurrealEngine"
SETTINGS_NAME = "Settings.json"
BOT_SECTION = "Botpack.DeathMatchPlus"
BOT_KEYS = ("MinPlayers", "InitialBots")
# Nothing in a class that matches this can add a URL option.
CLASS_PATTERN = re.compile(r"[A-Za-z0-9_]+\.[A-Za-z0-9_]+")
KEY_LEAF_ALLOWED = frozenset(b"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._-")


def class_option(player_class):
    """The URL option that sets the player's class; refuses anything that could add another option."""
    if not CLASS_PATTERN.fullmatch(player_class):
        raise ValueError(f"runpins: bad player class {player_class!r} (expected Package.Class)")
    return f"?Class={player_class}"


def pinned_url(map_name, game_text="", player_class=None, url_options=""):
    """map, game text, class, then the caller's options: the order the engine's URL parser sees."""
    option = class_option(player_class) if player_class else ""
    return f"{map_name}{game_text}{option}{url_options}"


def game_key(exe, version, root_folder):
    """The engine's folder key `<exe>-<version>-<leaf>` for an install folder (UserDirs::gameKey).

    The leaf is the folder's last component with each character outside [A-Za-z0-9._-] (a UTF-8
    sequence counting once) replaced by '_', cut to MAX_KEY_LEAF, and 'root' when nothing is left.
    """
    leaf = b""
    if root_folder:
        leaf = os.path.basename(os.path.normpath(os.fsencode(root_folder)))
    chars = []
    for byte in leaf:
        if 0x80 <= byte <= 0xBF:  # continuation byte of a character already replaced
            continue
        chars.append(chr(byte) if byte in KEY_LEAF_ALLOWED else "_")
    key = "".join(chars)[:MAX_KEY_LEAF] or "root"
    return f"{exe}-{version}-{key}"


def pin_bots(ini_text, count):
    """The install's ini text with MinPlayers=0 and InitialBots=<count> in [Botpack.DeathMatchPlus].

    Every other byte stays, line ends included. A missing section or key is an error: appending
    would write a key the engine may read from another place.
    """
    wanted = {"MinPlayers": 0, "InitialBots": count}
    lines = ini_text.splitlines(keepends=True)
    inside = False
    seen = set()
    for i, line in enumerate(lines):
        body = line.rstrip("\r\n")
        if body.startswith("["):
            inside = body.strip().lower() == f"[{BOT_SECTION}]".lower()
            continue
        name, equals, _ = body.partition("=")
        if inside and equals and name.strip() in wanted and name.strip() not in seen:
            seen.add(name.strip())
            lines[i] = f"{name}={wanted[name.strip()]}{line[len(body):]}"
    missing = [k for k in BOT_KEYS if k not in seen]
    if missing:
        raise ValueError(f"runpins: [{BOT_SECTION}] of the ini lacks {', '.join(missing)}")
    return "".join(lines)


def host_config_root(env=None):
    """The config root the engine uses for the caller's environment: XDG_CONFIG_HOME when absolute."""
    env = os.environ if env is None else env
    xdg = env.get("XDG_CONFIG_HOME", "")
    if xdg.startswith("/"):
        return Path(xdg)
    return Path(env.get("HOME") or Path.home()) / ".config"


def _inside(path, folder):
    return path == folder or folder in path.parents


def game_config_folder(game, kind, config_root, user_dir=None):
    """The folder the engine reads a game's per-user ini from, and the ini's name under it."""
    exe, version = KINDS[kind]
    # Lexical paths, as the engine prints them: a resolved symlink would name another folder.
    key = game_key(exe, version, os.path.abspath(game))
    if user_dir is not None:
        return Path(os.path.abspath(user_dir)) / key
    return Path(os.path.abspath(config_root)) / APP_DIR / key


def prepare(game, kind, config_root, bots=None, user_dir=None, host_root=None):
    """Make the run's config root: Settings.json copied once, and for `ut` the bot pin written.

    Returns (config folder or None, pin file or None). A user directory holds the per-game
    folder instead of the config root, so with one the pin goes there. Nothing is written
    under the game folder or the host's own config folder, and a file already in the root is
    left alone except the pin file, which is rewritten from the install's ini.
    """
    given_game, given_root = game, config_root
    game = Path(game).resolve()
    root = Path(config_root).resolve()
    host = Path(host_root).resolve() if host_root is not None else host_config_root().resolve()
    if _inside(root, game):
        raise ValueError(f"runpins: config root {root} is inside the game folder {game}")
    if _inside(root, host / APP_DIR) or root == host:
        raise ValueError(f"runpins: config root {root} is the host's own config folder")
    if user_dir is not None and _inside(Path(user_dir).resolve(), game):
        raise ValueError(f"runpins: user directory {user_dir} is inside the game folder {game}")
    if bots is not None and kind != "ut":
        raise ValueError(f"runpins: a bot count is pinned for kind ut only, not {kind}")
    if kind is not None and kind not in KINDS:
        raise ValueError(f"runpins: unknown kind {kind!r} (expected one of {', '.join(KINDS)})")
    target = root / APP_DIR
    target.mkdir(parents=True, exist_ok=True)
    settings = target / SETTINGS_NAME
    host_settings = host / APP_DIR / SETTINGS_NAME
    if not settings.exists():
        if host_settings.is_file():
            shutil.copyfile(host_settings, settings)
        else:
            print(f"runpins: no {host_settings} to copy; the engine will use its default settings",
                  file=sys.stderr)
    if bots is None:
        return None, None
    ini = game / "System" / "UnrealTournament.ini"
    try:
        text = ini.read_bytes().decode("latin-1")
    except OSError as e:
        raise ValueError(f"runpins: cannot read the install's ini {ini}: {e}") from e
    pinned = pin_bots(text, bots)
    folder = game_config_folder(given_game, kind, given_root, user_dir)
    folder.mkdir(parents=True, exist_ok=True)
    pin_file = folder / "SE-UnrealTournament.ini"
    pin_file.write_bytes(pinned.encode("latin-1"))
    return folder, pin_file


class LevelInfo(NamedTuple):
    text: str
    by_name: bool


class Pins(NamedTuple):
    config_dir: str | None
    init_options: list
    level_info: list


CONFIG_LINE = re.compile(r"User directories: config=(.*) data=")
INIT_LINE = re.compile(r"GameInfo\.InitGame: InitGame: (\S*)")
LEVEL_LINE = re.compile(r"Level info: (.*)")


def read_pins(lines):
    """The config folder, each InitGame option string and each level-info line of an engine log."""
    config_dir = None
    options = []
    levels = []
    for line in lines:
        if (m := CONFIG_LINE.search(line)) and config_dir is None:
            config_dir = m.group(1)
        elif m := INIT_LINE.search(line):
            options.append(m.group(1))
        elif m := LEVEL_LINE.search(line):
            levels.append(LevelInfo(m.group(1), "(by name;" in m.group(1)))
    return Pins(config_dir, options, levels)


def check_pins(lines, expected_config, player_class=None):
    """(ok, report lines) for a run's pins read back from its engine log.

    `expected_config` is the folder the run was meant to use, or a (parent, key prefix) pair when
    the install's folder name is not known to the caller.
    """
    pins = read_pins(lines)
    ok = True
    report = []
    seen = pins.config_dir
    if isinstance(expected_config, tuple):
        parent, prefix = expected_config
        good = seen is not None and Path(seen).parent == Path(parent) and Path(seen).name.startswith(prefix)
        wanted = f"{parent}/{prefix}*"
    else:
        good = seen is not None and Path(seen) == Path(expected_config)
        wanted = str(expected_config)
    if good:
        report.append("CONFIG_DIR ok")
    else:
        ok = False
        report.append(f"CONFIG_DIR MISMATCH expected {wanted} observed {seen}")
    if player_class is None:
        report.append("CLASS not requested")
    elif any(f"Class={player_class}" in option.split("?") for option in pins.init_options):
        report.append("CLASS ok")
    else:
        ok = False
        report.append(f"CLASS MISSING {player_class} in InitGame options {pins.init_options}")
    report.append(f"LEVEL_INFO {len(pins.level_info)} BY_NAME {sum(1 for lv in pins.level_info if lv.by_name)}")
    return ok, report


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="command", required=True)
    prep = sub.add_parser("prepare", help="make a run's config root")
    prep.add_argument("--game", required=True)
    prep.add_argument("--kind", required=True, choices=sorted(KINDS))
    prep.add_argument("--config-root", required=True)
    prep.add_argument("--bots", type=int)
    prep.add_argument("--user-dir")
    chk = sub.add_parser("check", help="read a run's pins back from its engine log")
    chk.add_argument("--engine-log", required=True)
    chk.add_argument("--config-root", required=True)
    chk.add_argument("--kind", required=True, choices=sorted(KINDS))
    chk.add_argument("--game")
    chk.add_argument("--user-dir")
    chk.add_argument("--class", dest="player_class")
    chk.add_argument("--bots", type=int)
    args = ap.parse_args(argv)

    if args.command == "prepare":
        try:
            folder, pin_file = prepare(args.game, args.kind, args.config_root, args.bots, args.user_dir)
        except ValueError as e:
            sys.exit(str(e))
        print(f"CONFIG_ROOT {Path(args.config_root).resolve()}")
        if pin_file is not None:
            print(f"PIN_FILE {pin_file}")
        return 0

    try:
        lines = Path(args.engine_log).read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError as e:
        sys.exit(f"runpins: cannot read the engine log {args.engine_log}: {e}")
    root = Path(args.config_root).resolve()
    exe, version = KINDS[args.kind]
    if args.game:
        expected = game_config_folder(args.game, args.kind, root, args.user_dir)
    else:
        parent = Path(args.user_dir).resolve() if args.user_dir else root / APP_DIR
        expected = (parent, f"{exe}-{version}-")
    ok, report = check_pins(lines, expected, args.player_class)
    if args.bots is not None:
        folder = Path(read_pins(lines).config_dir or "")
        ini = folder / "SE-UnrealTournament.ini"
        try:
            text = ini.read_bytes().decode("latin-1")
            found = pin_bots(text, args.bots) == text
        except (OSError, ValueError):
            found = False
        report.append(f"BOTS {args.bots} ok" if found else f"BOTS MISMATCH {args.bots} not in {ini}")
        ok = ok and found
    print("\n".join(report))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
