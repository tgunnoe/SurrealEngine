import contextlib
import io
import random
import re
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import runpins

# The two keys of the section the pin writes, in an ini with the line ends the games' own files have.
UT_INI = (
    "[Botpack.CTFGame]\r\nMaxTeams=4\r\nMinPlayers=7\r\n\r\n"
    "[Botpack.DeathMatchPlus]\r\nbMultiPlayerBots=True\r\nMinPlayers=4\r\nInitialBots=10\r\n"
    "bChangeLevels=True\r\n\r\n"
    "[Botpack.Domination]\r\nInitialBots=3\r\n"
)

ENGINE_LOG = [
    "[0.000000] <Surreal Engine>: Game: Unreal (Version: 226b)",
    "[0.000000] <Surreal Engine>: User directories: config=/x/SurrealEngine/Unreal-226b-Unreal_Gold "
    "data=/y/SurrealEngine/Unreal-226b-Unreal_Gold",
    "[0.000000] <Surreal Engine>: Level info: LevelInfo0 (the level's first actor) in Entry",
    "[0.000000] GameInfo.InitGame: InitGame: ?Name=Player?Class=Unreali.MaleTwo?Difficulty=1",
    "[0.500000] <Surreal Engine>: Level info: LevelInfo1 (by name; the first actor is Brush) in Vortex2",
    "[0.500000] GameInfo.InitGame: InitGame: ?Name=Player?Class=UPak.UPakFemaleOne?Difficulty=1",
]


def ut_install(root):
    """A fixture install folder with an Unreal Tournament ini, and the ini's bytes."""
    game = root / "Unreal Tournament GOTY"
    (game / "System").mkdir(parents=True)
    raw = UT_INI.encode("latin-1")
    (game / "System" / "UnrealTournament.ini").write_bytes(raw)
    return game, raw


def host_root(root):
    """A fixture host config root holding a Settings.json."""
    host = root / "host"
    (host / "SurrealEngine").mkdir(parents=True)
    (host / "SurrealEngine" / "Settings.json").write_text('{"device": "OpenGL"}\n')
    return host


class ClassOptionTests(unittest.TestCase):
    def test_class_option_builds_the_url_option(self):
        self.assertEqual(runpins.class_option("UPak.UPakFemaleOne"), "?Class=UPak.UPakFemaleOne")

    def test_class_option_refuses_anything_that_could_add_an_option(self):
        for bad in ["UPak.UPakFemaleOne?x=1", "UPak", "a.b.c", "a b.c", "", "&"]:
            with self.subTest(bad=bad):
                with self.assertRaises(ValueError):
                    runpins.class_option(bad)

    def test_pinned_url_puts_the_class_after_the_game_text(self):
        self.assertEqual(runpins.pinned_url("Vortex2", "?game=Botpack.CTFGame", "UPak.UPakFemaleOne", "?x=1"),
                         "Vortex2?game=Botpack.CTFGame?Class=UPak.UPakFemaleOne?x=1")
        self.assertEqual(runpins.pinned_url("Vortex2", "?game=G.C", None, "?x=1"), "Vortex2?game=G.C?x=1")
        self.assertEqual(runpins.pinned_url("Vortex2"), "Vortex2")


class GameKeyTests(unittest.TestCase):
    def test_game_key_matches_the_engine_rule(self):
        # Each expected string is the one of tests/userdirs/UserDirsTests.cpp.
        leaf40 = "abcdefghijklmnopqrstuvwxyz0123456789ABCD"
        cases = [
            ("DeusEx", "1112fm", "/srv/u/Games/gog/gmdx/drive_c/GOG Games/GMDX", "DeusEx-1112fm-GMDX"),
            ("Unreal", "226b", "/srv/u/Games/gog/unreal-gold/drive_c/GOG Games/Unreal Gold",
             "Unreal-226b-Unreal_Gold"),
            ("UnrealTournament", "436",
             "/srv/u/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY",
             "UnrealTournament-436-Unreal_Tournament_GOTY"),
            ("UnrealTournament", "436", "/srv/u/Games/gog/deus-ex/drive_c/GOG Games/Deus Ex GOTY",
             "UnrealTournament-436-Deus_Ex_GOTY"),
            ("Unreal", "226b", "/g/Unreal Gold/", "Unreal-226b-Unreal_Gold"),
            ("Unreal", "226b", "/g/Unreal Gold//", "Unreal-226b-Unreal_Gold"),
            ("Unreal", "226b", "/g/x/../Unreal Gold", "Unreal-226b-Unreal_Gold"),
            ("Unreal", "226b", "/g/./Unreal Gold", "Unreal-226b-Unreal_Gold"),
            ("DeusEx", "1112fm", "/g/My Game (v1.2) [x]!", "DeusEx-1112fm-My_Game__v1.2___x__"),
            ("DeusEx", "1112fm", "/g/a-b_c.d", "DeusEx-1112fm-a-b_c.d"),
            ("DeusEx", "1112fm", "/g/Café", "DeusEx-1112fm-Caf_"),
            ("Unreal", "226b", "/g/" + leaf40, "Unreal-226b-abcdefghijklmnopqrstuvwxyz012345"),
            ("Unreal", "226b", "/g/abcdefghijklmnopqrstuvwxyz012345",
             "Unreal-226b-abcdefghijklmnopqrstuvwxyz012345"),
            ("Unreal", "226b", "", "Unreal-226b-root"),
            ("Unreal", "226b", "/", "Unreal-226b-root"),
        ]
        for exe, version, folder, expected in cases:
            with self.subTest(folder=folder):
                self.assertEqual(runpins.game_key(exe, version, folder), expected)

    def test_game_key_keeps_its_shape_for_random_leaves(self):
        rng = random.Random(98)
        alphabet = "abcXYZ019 ._-()[]!é中"
        for _ in range(200):
            leaf = "x" + "".join(rng.choice(alphabet) for _ in range(rng.randint(0, 39)))
            key = runpins.game_key("Unreal", "226b", f"/g/{leaf}")
            with self.subTest(leaf=leaf):
                match = re.fullmatch(r"Unreal-226b-([A-Za-z0-9._-]+)", key)
                self.assertIsNotNone(match, key)
                self.assertLessEqual(len(match.group(1)), 32)


class BotPinTests(unittest.TestCase):
    def test_bot_pin_changes_exactly_two_lines(self):
        for count in (0, 2, 10):
            pinned = runpins.pin_bots(UT_INI, count)
            expected = UT_INI.replace("MinPlayers=4\r\nInitialBots=10\r\n",
                                      f"MinPlayers=0\r\nInitialBots={count}\r\n")
            with self.subTest(count=count):
                self.assertEqual(pinned, expected)
                self.assertEqual(sum(b != a for b, a in zip(UT_INI.splitlines(keepends=True),
                                                            pinned.splitlines(keepends=True), strict=True)),
                                 1 if count == 10 else 2)

    def test_bot_pin_refuses_an_ini_without_the_keys(self):
        for text in ["[Botpack.CTFGame]\r\nMinPlayers=0\r\n",
                     "[Botpack.DeathMatchPlus]\r\nMinPlayers=0\r\n",
                     "[Botpack.DeathMatchPlus]\r\nInitialBots=10\r\n[Botpack.Domination]\r\nMinPlayers=0\r\n"]:
            with self.subTest(text=text):
                with self.assertRaises(ValueError):
                    runpins.pin_bots(text, 0)

    def test_bot_pin_is_idempotent(self):
        once = runpins.pin_bots(UT_INI, 2)
        self.assertEqual(runpins.pin_bots(once, 2), once)


class PrepareTests(unittest.TestCase):
    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.root = Path(tmp.name)
        self.game, self.ini_bytes = ut_install(self.root)
        self.host = host_root(self.root)
        self.config = self.root / "run" / "xdg"

    def test_prepare_writes_the_pin_in_the_engines_folder(self):
        folder, pin_file = runpins.prepare(self.game, "ut", self.config, bots=2, host_root=self.host)
        expected = self.config / "SurrealEngine" / "UnrealTournament-436-Unreal_Tournament_GOTY"
        self.assertEqual((folder, pin_file), (expected, expected / "SE-UnrealTournament.ini"))
        self.assertTrue(pin_file.is_file())
        written = UT_INI.replace("MinPlayers=4", "MinPlayers=0").replace("InitialBots=10", "InitialBots=2")
        self.assertEqual(pin_file.read_bytes(), written.encode("latin-1"))
        self.assertEqual((self.game / "System" / "UnrealTournament.ini").read_bytes(), self.ini_bytes)
        self.assertEqual(sorted(p.name for p in (self.game / "System").iterdir()), ["UnrealTournament.ini"])
        # A user directory holds the per-game folder instead of the config root.
        user = self.root / "user"
        folder, pin_file = runpins.prepare(self.game, "ut", self.config, bots=0, user_dir=user,
                                           host_root=self.host)
        self.assertEqual(folder, user / "UnrealTournament-436-Unreal_Tournament_GOTY")
        self.assertTrue(pin_file.is_file())

    def test_prepare_copies_settings_once_and_never_overwrites(self):
        runpins.prepare(self.game, "unreal", self.config, host_root=self.host)
        settings = self.config / "SurrealEngine" / "Settings.json"
        self.assertTrue(settings.is_file())
        self.assertEqual(settings.read_text(), '{"device": "OpenGL"}\n')
        settings.write_text("kept\n")
        (self.host / "SurrealEngine" / "Settings.json").write_text("changed\n")
        runpins.prepare(self.game, "unreal", self.config, host_root=self.host)
        self.assertEqual(settings.read_text(), "kept\n")
        self.assertEqual((self.host / "SurrealEngine" / "Settings.json").read_text(), "changed\n")

    def test_prepare_refuses_unsafe_roots(self):
        with self.assertRaises(ValueError):
            runpins.prepare(self.game, "ut", self.game / "System" / "xdg", bots=0, host_root=self.host)
        with self.assertRaises(ValueError):
            runpins.prepare(self.game, "ut", self.host, bots=0, host_root=self.host)
        for kind in ("unreal", "deusex"):
            with self.subTest(kind=kind), self.assertRaises(ValueError):
                runpins.prepare(self.game, kind, self.config, bots=0, host_root=self.host)
        self.assertFalse(self.config.exists())
        self.assertEqual(sorted(p.name for p in (self.game / "System").iterdir()), ["UnrealTournament.ini"])


class ReadPinsTests(unittest.TestCase):
    def test_read_pins_parses_the_engine_log(self):
        pins = runpins.read_pins(ENGINE_LOG)
        self.assertEqual(pins.config_dir, "/x/SurrealEngine/Unreal-226b-Unreal_Gold")
        self.assertEqual(pins.init_options, ["?Name=Player?Class=Unreali.MaleTwo?Difficulty=1",
                                             "?Name=Player?Class=UPak.UPakFemaleOne?Difficulty=1"])
        self.assertEqual([(lv.text, lv.by_name) for lv in pins.level_info],
                         [("LevelInfo0 (the level's first actor) in Entry", False),
                          ("LevelInfo1 (by name; the first actor is Brush) in Vortex2", True)])

    def test_check_reports_a_mismatch(self):
        folder = "/x/SurrealEngine/Unreal-226b-Unreal_Gold"
        ok, report = runpins.check_pins(ENGINE_LOG, folder, "UPak.UPakFemaleOne")
        self.assertEqual((ok, report), (True, ["CONFIG_DIR ok", "CLASS ok", "LEVEL_INFO 2 BY_NAME 1"]))
        ok, report = runpins.check_pins(ENGINE_LOG, "/z/SurrealEngine/Unreal-226b-Unreal_Gold", None)
        self.assertFalse(ok)
        self.assertIn("/x/SurrealEngine/Unreal-226b-Unreal_Gold", report[0])
        ok, report = runpins.check_pins(ENGINE_LOG, folder, "UPak.UPakMaleOne")
        self.assertFalse(ok)
        self.assertTrue(report[1].startswith("CLASS MISSING"))
        # A class that only prefixes the option's value is not the option.
        self.assertFalse(runpins.check_pins(ENGINE_LOG, folder, "UPak.UPakFemale")[0])


if __name__ == "__main__":
    unittest.main()
