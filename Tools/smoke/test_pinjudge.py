import contextlib
import io
import re
import shlex
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import pinjudge

SCENARIOS = Path(__file__).resolve().parent / "scenarios"
READMES = [SCENARIOS / "movers" / "README.md", SCENARIOS / "zones" / "README.md"]


def answer(t, form, name, at=(0.0, 0.0, 0.0)):
    return (f"[{t:.6f}] <Surreal Engine>: call: {form} resolved to {name} at "
            f"({at[0]:.1f}, {at[1]:.1f}, {at[2]:.1f})")


def trace(t, obj, func, args, cls="Mover"):
    return f"[{t:.6f}] Engine.{cls}: trace: {obj} {cls} None: {func}({args})"


def place(t, at):
    return f"[{t:.6f}] <Surreal Engine>: place: PLACE {at} -> ({at[0]:.1f}, {at[1]:.1f}, {at[2]:.1f})"


def smoke(fps="30.0", summary=True):
    return "GAME x 1\nMAP A\nLOGIN\n" + f"FPS {fps}\n" + ("SUMMARY script_errors=0\n" if summary else "")


def scheduled_answers(pin_id, name, where=lambda t, form: (0.0, 0.0, 0.0), only=None):
    """One answer per probe the pin schedules, for the player's form and the movers' forms."""
    pin = pinjudge.PINS[pin_id]
    lines = []
    for t, command in pinjudge.exec_items(pin.exec):
        if command.startswith("call LineOfSightTo "):
            form = command.removeprefix("call LineOfSightTo ")
            if only is None or form == only:
                who = name if form == f"class:{pin.player_class}" else form.removeprefix("name:")
                lines.append(answer(t, form, who, where(t, form)))
    return lines


def judged_detail(pin_id, extra=(), name="Player0", where=lambda t, form: (0.0, 0.0, 0.0), smoke_text=None,
                  answers=None):
    lines = scheduled_answers(pin_id, name, where) if answers is None else answers
    ordered = sorted(list(lines) + list(extra), key=lambda s: float(s[1:s.index("]")]))
    engine = "\n".join(ordered) + "\n"
    return pinjudge.judge(pinjudge.PINS[pin_id], smoke() if smoke_text is None else smoke_text, engine)


def judged(pin_id, extra=(), name="Player0", where=lambda t, form: (0.0, 0.0, 0.0), smoke_text=None,
           answers=None):
    return judged_detail(pin_id, extra, name, where, smoke_text, answers)[0]


class PlayerNameTests(unittest.TestCase):
    def test_player_name_from_the_probe_answers(self):
        for leaf, name in [("MaleTwo", "MaleTwo0"), ("UPakFemaleOne", "UPakFemaleOne0"),
                           ("TMale2", "TMale20"), ("JCDentonMale", "JCDentonMale0")]:
            run = pinjudge.read_run("", "\n".join([answer(5.0, "name:Mover44", "Mover44"),
                                                   answer(5.25, f"class:{leaf}", name),
                                                   answer(5.5, f"class:{leaf}", name)]))
            with self.subTest(leaf=leaf):
                self.assertEqual(pinjudge.player_name(run, leaf), name)

    def test_player_name_with_two_objects_follows_the_placement(self):
        before = [answer(7.0, "class:TMale2", "TMale2Bot0", (900.0, 900.0, 0.0))]
        after = [answer(8.5, "class:TMale2", "TMale2Bot0", (900.0, 900.0, 0.0)),
                 answer(9.0, "class:TMale2", "TMale20", (110.0, 205.0, 300.0))]
        placed = place(8.0, (100.0, 200.0, 300.0))
        read = lambda *lines: pinjudge.read_run("", "\n".join(lines))  # noqa: E731
        self.assertEqual(pinjudge.player_name(read(*before, placed, *after), "TMale2"), "TMale20")
        # Neither the order of the answers nor the placed object's being the later one matters.
        swapped = [answer(8.5, "class:TMale2", "TMale20", (110.0, 205.0, 300.0)),
                   answer(9.0, "class:TMale2", "TMale2Bot0", (900.0, 900.0, 0.0))]
        self.assertEqual(pinjudge.player_name(read(placed, *swapped), "TMale2"), "TMale20")
        self.assertIsNone(pinjudge.player_name(read(*before, *after), "TMale2"))
        far = [answer(8.5, "class:TMale2", "TMale2Bot0", (900.0, 900.0, 0.0)),
               answer(9.0, "class:TMale2", "TMale20", (300.0, 205.0, 300.0))]
        self.assertIsNone(pinjudge.player_name(read(placed, *far), "TMale2"))
        both = [answer(8.5, "class:TMale2", "TMale2Bot0", (120.0, 210.0, 300.0)),
                answer(9.0, "class:TMale2", "TMale20", (110.0, 205.0, 300.0))]
        self.assertIsNone(pinjudge.player_name(read(placed, *both), "TMale2"))


def readme_rows():
    rows = {}
    for path in READMES:
        for line in path.read_text(encoding="utf-8").splitlines():
            cells = [c.strip() for c in line.strip().strip("|").split(" | ")]
            if len(cells) == 7 and re.fullmatch(r"`(mv|zn)-[a-z0-9-]+`", cells[0]):
                rows[cells[0].strip("`")] = (path, [c.strip("`") for c in cells])
    return rows


class ReadmeTests(unittest.TestCase):
    def test_every_pin_row_matches_the_readme(self):
        rows = readme_rows()
        self.assertEqual(sorted(rows), sorted(pinjudge.PINS))
        self.assertEqual(len(pinjudge.PINS), 12)
        for pin_id, pin in pinjudge.PINS.items():
            path, cells = rows[pin_id]
            with self.subTest(pin=pin_id):
                self.assertEqual(path.parent.name, "movers" if pin_id.startswith("mv-") else "zones")
                _, game, url, exec_text, trace_text, exit_after, _ = cells
                self.assertEqual((url, exec_text, trace_text, int(exit_after)),
                                 (pin.full_url(), pin.exec, pin.trace, pin.exit_after))
                args = shlex.split(" ".join(shlex.quote(a) for a in pin.args()))
                self.assertIn(f"--url={url}", args)
                self.assertIn(f"--exec={exec_text}", args)
                self.assertIn(f"--trace-script={trace_text}", args)
                self.assertIn(f"--exit-after={exit_after}", args)
                self.assertIn("--frame-limit=0", args)


def at_times(positions):
    """A `where` that puts the player at the given (x, y) at each listed probe time, else at the origin."""
    return lambda t, form: (*positions.get(round(t, 2), (0.0, 0.0)), 0.0)


class SlideContactTests(unittest.TestCase):
    """The slide is read from the player's first contact with the mover's face, not from fixed times."""

    def bump(self, t, who="Player0"):
        return trace(t, "Mover44", "Bump", who)

    def test_the_slide_is_read_from_the_first_bump_of_the_player(self):
        for moved, expected in [(99.9, "PIN-BROKEN"), (100.0, "PIN-HELD")]:
            # The old probes (11.5 s and 13.0 s) and the 1.5 s probe (13.25 s) read the opposite verdict.
            opposite = 0.0 if expected == "PIN-HELD" else 500.0
            where = at_times({11.75: (0.0, 0.0), 14.75: (moved, 0.0), 11.5: (0.0, 0.0), 13.0: (opposite, 0.0),
                              13.25: (opposite, 0.0)})
            with self.subTest(moved=moved):
                self.assertEqual(judged("mv-slide-ug", [self.bump(11.6)], where=where), expected)

    def test_the_slide_window_moves_with_the_contact(self):
        where = at_times({11.25: (0.0, 0.0), 14.25: (200.0, 0.0), 11.75: (200.0, 0.0), 14.75: (200.0, 0.0)})
        self.assertEqual(judged("mv-slide-ug", [self.bump(11.1)], where=where), "PIN-HELD")
        self.assertEqual(judged("mv-slide-ug", [self.bump(11.6)], where=where), "PIN-BROKEN")

    def test_only_a_bump_of_the_player_after_the_hold_starts_is_the_contact(self):
        # Once the player has moved off, it stays 150 units away, so a window of any length reads a slide.
        where = lambda t, form: (150.0 if t >= 11.5 else 0.0, 0.0, 0.0)  # noqa: E731
        elsewhere = [self.bump(10.1, "Other0"), self.bump(11.0, "Other0"), self.bump(9.5)]
        verdict, detail = judged_detail("mv-slide-ug", elsewhere, where=where)
        self.assertEqual(verdict, "PIN-BROKEN")
        self.assertTrue(any("no contact" in line for line in detail), detail)
        self.assertEqual(judged("mv-slide-ug", elsewhere + [self.bump(11.2)], where=where), "PIN-HELD")

    def test_a_slide_without_a_contact_is_broken_not_void(self):
        where = at_times({13.0: (500.0, 0.0)})
        verdict, detail = judged_detail("mv-slide-ug", where=where)
        self.assertEqual(verdict, "PIN-BROKEN")
        self.assertTrue(any("no contact" in line for line in detail), detail)
        verdict, detail = judged_detail("mv-slide-ug", [self.bump(13.5)], where=where)
        self.assertEqual(verdict, "PIN-BROKEN")
        self.assertTrue(any("too late" in line for line in detail), detail)
        # A log with no probe answers, or with the answer at t1 missing, is unreadable, not a broken slide.
        self.assertEqual(judged("mv-slide-ug", [self.bump(11.2)], answers=[]), "VOID")
        every = scheduled_answers("mv-slide-ug", "Player0")
        without_t1 = [line for line in every if not (line.startswith("[14.250000]") and "Player0" in line)]
        self.assertEqual(judged("mv-slide-ug", [self.bump(11.2)], answers=without_t1), "VOID")

    def test_the_first_of_two_bumps_of_the_player_is_the_contact(self):
        # Two contacts: the window of the first (11.25 s to 14.25 s) reads a slide, that of the last
        # (11.75 s to 14.75 s) reads none, and the other way round for the second fixture.
        slid_early = at_times({11.25: (0.0, 0.0), 14.25: (200.0, 0.0), 11.75: (200.0, 0.0), 14.75: (200.0, 0.0)})
        slid_late = at_times({11.25: (0.0, 0.0), 14.25: (0.0, 0.0), 11.75: (0.0, 0.0), 14.75: (200.0, 0.0)})
        bumps = [self.bump(11.1), self.bump(11.6)]
        self.assertEqual(judged("mv-slide-ug", bumps, where=slid_early), "PIN-HELD")
        self.assertEqual(judged("mv-slide-ug", bumps, where=slid_late), "PIN-BROKEN")

    def test_only_a_bump_by_the_pins_mover_is_the_contact(self):
        # Reading the Bump of another mover at 11.0 s as the contact would pair 11.0 s with 14.0 s.
        where = at_times({11.75: (0.0, 0.0), 14.0: (0.0, 0.0), 12.0: (200.0, 0.0), 14.75: (200.0, 0.0)})
        other = trace(11.0, "Mover7", "Bump", "Player0")
        self.assertEqual(judged("mv-slide-ug", [other, self.bump(11.6)], where=where), "PIN-HELD")

    def test_a_probe_printed_a_millisecond_early_still_closes_the_window(self):
        # The engine prints a probe's time to six places after scheduling it to three, so the probe
        # for 14.25 s can be logged at 14.249 s and must still be the one 3.0 s after the one at 11.25 s.
        where = at_times({11.25: (0.0, 0.0), 14.25: (200.0, 0.0), 14.5: (0.0, 0.0)})
        lines = [line.replace("[14.250000]", "[14.249000]") if "Player0" in line else line
                 for line in scheduled_answers("mv-slide-ug", "Player0", where)]
        self.assertEqual(judged("mv-slide-ug", [self.bump(11.2)], answers=lines), "PIN-HELD")


class MoverJudgeTests(unittest.TestCase):
    def test_mover_judges_apply_the_recorded_thresholds(self):
        proj = lambda **n: [trace(10.0 + i, "Mover44", "Bump", "UPakFemaleOne0") for i in range(n.get("bump", 2))] + \
            [trace(10.0 + i, "Projectile", "HitWall", "?, Mover44") for i in range(n.get("hit", 2))] + \
            ([trace(11.0, "Projectile", "Touch", "Mover44")] if n.get("touch") else []) + \
            ([trace(11.0, "Mover44", "TakeDamage", "15, ?")] if n.get("damage") else [])
        for kwargs, expected in [({}, "PIN-HELD"), ({"hit": 1}, "PIN-BROKEN"), ({"bump": 1}, "PIN-BROKEN"),
                                 ({"touch": 1}, "PIN-BROKEN"), ({"damage": 1}, "PIN-BROKEN")]:
            with self.subTest(kwargs=kwargs):
                self.assertEqual(judged("mv-proj-ug", proj(**kwargs)), expected)

        def slide(pin_id, moved):
            pin = pinjudge.PINS[pin_id]
            if "mover" in pin.params:
                # A contact at 11.2 s reads from 11.25 s to 14.25 s; the probe 1.5 s after the first stays
                # at the first's position, so a window of 1.5 s would read no travel.
                contact = [trace(11.2, pin.params["mover"], "Bump", "Player0")]
                where = at_times({14.25: (moved, 0.0)})
            else:
                contact = []
                where = lambda t, form: (moved, 0.0, 0.0) if t == pin.params["t1"] else (0.0, 0.0, 0.0)  # noqa: E731
            return judged(pin_id, contact, where=where)
        for pin_id, moved, expected in [("mv-slide-ug", 99.9, "PIN-BROKEN"), ("mv-slide-ug", 100.0, "PIN-HELD"),
                                        ("mv-slide-dx", 14.9, "PIN-BROKEN"), ("mv-slide-dx", 15.0, "PIN-HELD")]:
            with self.subTest(pin=pin_id, moved=moved):
                self.assertEqual(slide(pin_id, moved), expected)

        crush = lambda n, killed=True: [trace(14.0 + i, "Mover44", "EncroachingOn", "Player0") for i in range(n)] + \
            ([trace(15.0, "Player0", "KilledBy", "Player0", "Pawn")] if killed else [])  # noqa: E731
        for n, killed, expected in [(0, True, "PIN-BROKEN"), (1, True, "PIN-HELD"), (1, False, "PIN-BROKEN"),
                                    (2, True, "PIN-BROKEN")]:
            with self.subTest(crush=n, killed=killed):
                self.assertEqual(judged("mv-enc-ug", crush(n, killed)), expected)

        def lift(n, returns):
            lines = []
            for i in range(n):
                lines.append(trace(10.0 + i, "Mover0", "EncroachingOn", "Player0"))
                if i < returns:
                    lines.append(trace(10.0 + i, "Mover0", "MakeGroupReturn", ""))
            return lines
        for n, returns, expected in [(2, 2, "PIN-BROKEN"), (3, 3, "PIN-HELD"), (3, 2, "PIN-BROKEN"),
                                     (4, 4, "PIN-BROKEN")]:
            with self.subTest(lift=n, returns=returns):
                self.assertEqual(judged("mv-enc-ut", lift(n, returns)), expected)

        def doors(crushed, by):
            return ([trace(10.0, "Mover16", "EncroachingOn", "Player0") for _ in range(crushed)] +
                    [trace(10.0, "Player0", "EncroachedBy", "DeusExMover16", "Pawn") for _ in range(by)])
        for crushed, by, expected in [(973, 973, "PIN-BROKEN"), (974, 974, "PIN-HELD"), (1460, 1460, "PIN-HELD"),
                                      (1461, 1461, "PIN-BROKEN"), (1000, 1002, "PIN-HELD"),
                                      (1000, 1003, "PIN-BROKEN")]:
            with self.subTest(crushed=crushed, by=by):
                self.assertEqual(judged("mv-enc-dx", doors(crushed, by)), expected)


class ZoneJudgeTests(unittest.TestCase):
    def pit(self, gap=0.0, second=None, far=False, warp=10.0, unwarps=1, warps=1):
        entered = [trace(warp, "WarpZoneInfo2", "ActorEntered", "Player0", "WarpZoneInfo") for _ in range(2)]
        extra = entered + [trace(warp, "WarpZoneInfo2", "UnWarp", "Player0", "WarpZoneInfo")] * unwarps + \
            [trace(warp, "WarpZoneInfo2", "Warp", "Player0", "WarpZoneInfo")] * warps
        if second is not None:
            extra += [trace(warp + second, "WarpZoneInfo2", "ActorEntered", "Player0", "WarpZoneInfo")] * 2
        if far:
            extra += [trace(warp + 0.1, "WarpZoneInfo3", "ActorEntered", "Player0", "WarpZoneInfo")] * 2
        where = lambda t, form: (786.8 + gap, -446.5, 487.0) if t > warp else (1186.8, -454.5, 600.0)  # noqa: E731
        return judged("zn-w1", extra, where=where)

    def test_zone_judges_apply_the_recorded_thresholds(self):
        self.assertEqual(self.pit(), "PIN-HELD")
        self.assertEqual(self.pit(second=4.9), "PIN-BROKEN")
        self.assertEqual(self.pit(second=5.1), "PIN-HELD")
        self.assertEqual(self.pit(far=True), "PIN-BROKEN")
        self.assertEqual(self.pit(gap=11.9), "PIN-HELD")
        self.assertEqual(self.pit(gap=12.1), "PIN-BROKEN")

        def nucleus(pin_id, z_at):
            where = lambda t, form: (0.0, 0.0, z_at(t - 8.0))  # noqa: E731
            return judged(pin_id, [place(8.0, (28.0, 3516.0, 444.0))], where=where)
        stands = lambda settle: (lambda dt: 424.0 if dt >= settle else 444.0)  # noqa: E731
        self.assertEqual(nucleus("zn-ctf1", stands(1.5)), "PIN-HELD")
        self.assertEqual(nucleus("zn-ctf2", stands(1.5)), "PIN-HELD")
        self.assertEqual(nucleus("zn-ctf1", stands(1.75)), "PIN-BROKEN")

        def falls(last):
            return lambda dt: 444.0 if dt < 0.5 else 380.0 if dt < 1.5 else last
        self.assertEqual(nucleus("zn-ctf3", falls(100.0)), "PIN-HELD")
        self.assertEqual(nucleus("zn-ctf3", falls(101.0)), "PIN-BROKEN")
        self.assertEqual(nucleus("zn-ctf3", stands(1.5)), "PIN-BROKEN")


    def test_the_warp_judge_wants_one_unwarp_and_one_warp(self):
        self.assertEqual(self.pit(unwarps=2), "PIN-BROKEN")
        self.assertEqual(self.pit(unwarps=0), "PIN-BROKEN")
        self.assertEqual(self.pit(warps=2), "PIN-BROKEN")

    def test_the_stand_judge_holds_a_player_to_two_units(self):
        def stands_at(z):
            where = lambda t, form: (0.0, 0.0, z if t - 8.0 >= 1.5 else 444.0)  # noqa: E731
            return judged("zn-ctf1", [place(8.0, (28.0, 3516.0, 444.0))], where=where)
        self.assertEqual(stands_at(426.0), "PIN-HELD")
        self.assertEqual(stands_at(426.5), "PIN-BROKEN")
        self.assertEqual(stands_at(421.5), "PIN-BROKEN")


class NameFromTheRunTests(unittest.TestCase):
    def test_judges_read_the_name_of_the_run_not_a_literal(self):
        def crush(probed, counted):
            extra = [trace(14.0, "Mover44", "EncroachingOn", counted), trace(15.0, "Player0", "KilledBy", counted, "Pawn")]
            return judged("mv-enc-ug", extra, name=probed)
        self.assertEqual(crush("MaleTwo0", "MaleTwo0"), "PIN-HELD")
        self.assertEqual(crush("UPakFemaleOne0", "UPakFemaleOne0"), "PIN-HELD")
        self.assertEqual(crush("MaleTwo0", "UPakFemaleOne0"), "PIN-BROKEN")


class VoidRunTests(unittest.TestCase):
    def test_a_void_run_is_not_judged(self):
        held = [trace(14.0, "Mover44", "EncroachingOn", "Player0"), trace(15.0, "Player0", "KilledBy", "Player0", "Pawn")]
        self.assertEqual(judged("mv-enc-ug", held), "PIN-HELD")
        self.assertEqual(judged("mv-enc-ug", held, smoke_text=smoke(summary=False)), "VOID")
        self.assertEqual(judged("mv-enc-ug", held, smoke_text=smoke(fps="0.5")), "VOID")
        self.assertEqual(judged("mv-enc-ug", held, answers=[]), "VOID")
        two = [answer(6.0, "class:UPakFemaleOne", "A0"), answer(6.5, "class:UPakFemaleOne", "B0")]
        self.assertEqual(judged("mv-enc-ug", held, answers=two), "VOID")


class SecondObjectTests(unittest.TestCase):
    """A second object of the player's class must never stand in for the player."""

    def swap(self, pin_id, at_t, other_at, where, extra=()):
        """The pin's answers for P0, with the one at `at_t` resolving to B0 at `other_at`."""
        lines = []
        for line in scheduled_answers(pin_id, "P0", where):
            if line.startswith(f"[{at_t:.6f}]") and "class:" in line:
                line = answer(at_t, line.split("call: ")[1].split(" ")[0], "B0", other_at)
            lines.append(line)
        return judged(pin_id, extra, answers=lines)

    def test_the_positional_judges_read_only_the_named_player(self):
        placed = [place(4.0, (0.0, 0.0, 0.0))]
        # The player stays where it was placed; one probe resolves to a second object 500 units away.
        self.assertEqual(self.swap("mv-slide-ug", 13.0, (500.0, 0.0, 0.0), lambda t, f: (0.0, 0.0, 0.0), placed),
                         "VOID")
        # The player sinks and never settles at 424; a later probe resolves to an object that does.
        stays = lambda t, f: (0.0, 0.0, 444.0)  # noqa: E731
        self.assertEqual(self.swap("zn-ctf1", 12.0, (0.0, 0.0, 424.0), stays,
                                   [place(8.0, (28.0, 3516.0, 444.0))]), "VOID")
        # The first probe after the warp resolves to an object on the column; the player is far from it.
        far = lambda t, f: (0.0, 0.0, 0.0)  # noqa: E731
        warp = [trace(10.0, "WarpZoneInfo2", "ActorEntered", "P0", "WarpZoneInfo")] * 2 + \
            [trace(10.0, "WarpZoneInfo2", "UnWarp", "P0", "WarpZoneInfo"),
             trace(10.0, "WarpZoneInfo2", "Warp", "P0", "WarpZoneInfo"), place(8.0, (1186.8, -454.5, 629.9))]
        self.assertNotEqual(self.swap("zn-w1", 10.5, (786.8, -446.5, 0.0), far, warp), "PIN-HELD")


class JudgeCommandTests(unittest.TestCase):
    def run_judge(self, extra, summary=True):
        lines = scheduled_answers("mv-enc-ug", "Player0") + list(extra)
        ordered = sorted(lines, key=lambda s: float(s[1:s.index("]")]))
        with tempfile.TemporaryDirectory() as tmp:
            smoke_path, engine_path = Path(tmp) / "run.log", Path(tmp) / "engine.txt"
            smoke_path.write_text(smoke(summary=summary))
            engine_path.write_text("\n".join(ordered) + "\n")
            out = io.StringIO()
            with contextlib.redirect_stdout(out):
                code = pinjudge.main(["judge", "mv-enc-ug", str(smoke_path), str(engine_path)])
        return code, out.getvalue().splitlines()[0]

    def test_judge_prints_the_verdict_and_exits_by_it(self):
        held = [trace(14.0, "Mover44", "EncroachingOn", "Player0"), trace(15.0, "Player0", "KilledBy", "Player0", "Pawn")]
        self.assertEqual(self.run_judge(held), (0, "PIN-HELD mv-enc-ug"))
        self.assertEqual(self.run_judge([]), (1, "PIN-BROKEN mv-enc-ug"))
        self.assertEqual(self.run_judge(held, summary=False), (2, "VOID mv-enc-ug"))


if __name__ == "__main__":
    unittest.main()
