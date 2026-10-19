import random
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import saveload

# Engine-log lines are copied from real runs, trimmed to the lines the parser
# reads and the noise around them.
UG_SAVE = [
    "[0.000000] <Surreal Engine>: Game: Unreal (Version: 226b)",
    "[0.000000] GameInfo.Login: Login: Player",
    "[5.998648] UPakPlayer.Summon: Summon: fabricated a UnrealShare.Stinger",
    "[11.999935] PlayerPawn.ShowInventory:    Weapon: UnrealShare.Stinger",
    "[11.999935] PlayerPawn.ShowInventory: Inv: Vortex2.StingerAmmo0",
    "[11.999935] PlayerPawn.ShowInventory: Inv: Vortex2.Stinger0",
    "[13.999454] <Surreal Engine>: Saved game to Unreal-226b-Unreal_Gold/Save/Save9.usa",
    "[39.624744] <Surreal Engine>: Shutting down...",
]

UG_LOAD_TRAVEL = [
    "[0.000000] GameInfo.Login: Login: Player",
    "[4.999555] <Surreal Engine>: call ClientTravel returned nothing",
    "[4.999555] <Surreal Engine>: Loaded game from slot 9: Vortex2",
    "[18.998175] PlayerPawn.ShowInventory:    Weapon: UnrealShare.Stinger",
    "[18.998175] PlayerPawn.ShowInventory: Inv: Save9.StingerAmmo0",
    "[18.998175] PlayerPawn.ShowInventory: Inv: Save9.Stinger0",
    "[23.998623] <Surreal Engine>: call ClientTravel returned nothing",
    "[23.998623] <Surreal Engine>: Client travel to Nyleve?Name=Player?Class=UPak.UPakFemaleOne?Difficulty=1",
    "[23.998623] <Surreal Engine>: Skipping unknown property PlayerName",
    "[20.000990] PlayerPawn.ShowInventory:    Weapon: UnrealShare.Stinger",
    "[20.000990] PlayerPawn.ShowInventory: Inv: NyLeve.DefaultAmmo0",
    "[20.000990] PlayerPawn.ShowInventory: Inv: NyLeve.DispersionPistol0",
    "[20.000990] PlayerPawn.ShowInventory: Inv: NyLeve.Stinger0",
    "[20.000990] PlayerPawn.ShowInventory: Inv: NyLeve.StingerAmmo0",
    "[25.998623] <Surreal Engine>: Saved game to Unreal-226b-Unreal_Gold/Save/Save2.usa",
]

UT_TRAVEL = [
    "[5.499887] <Surreal Engine>: call ClientTravel returned nothing",
    "[5.499887] <Surreal Engine>: Loaded game from slot 8: DM-Deck16][",
    "[10.597887] PlayerPawn.ShowInventory:    Weapon: Botpack.enforcer",
    "[10.597887] PlayerPawn.ShowInventory: Inv: Save8.ImpactHammer0 state None",
    "[10.597887] PlayerPawn.ShowInventory: Inv: Save8.miniammo0 state Idle2",
    "[10.597887] PlayerPawn.ShowInventory: Inv: Save8.enforcer0 state Idle",
    "[15.697407] <Surreal Engine>: Client travel to DM-Turbine?Name=Player?NumBots=0",
    "[15.697407] GameInfo.Login: Login: Player1",
    "[15.697407] <Surreal Engine>: trace: ImpactHammer0 ImpactHammer Pickup: TravelPreAccept()",
    "[15.697407] Botpack.ImpactHammer: trace: ImpactHammer0 ImpactHammer Pickup: TravelPreAccept()",
    "[15.697407] <Surreal Engine>: trace: enforcer0 enforcer Pickup: TravelPreAccept()",
    "[15.697407] Botpack.enforcer: trace: enforcer0 enforcer Pickup: TravelPreAccept()",
    "[15.697407] <Surreal Engine>: trace: Miniammo0 Miniammo Pickup: TravelPreAccept()",
    "[15.697407] Botpack.Miniammo: trace: Miniammo0 Miniammo Pickup: TravelPreAccept()",
    "[25.582748] PlayerPawn.ShowInventory:    Weapon: Botpack.enforcer",
    "[25.582748] PlayerPawn.ShowInventory: Inv: DM-Turbine.ImpactHammer1 state None",
    "[25.582748] PlayerPawn.ShowInventory: Inv: DM-Turbine.enforcer1 state Idle",
]

DX_LOAD = [
    "[4.991980] <Surreal Engine>: Loaded game from slot 2: 01_NYC_UNATCOHQ",
    "[9.991980] PlayerPawn.ShowInventory: Inv: 01_NYC_UNATCOHQ.NanoKeyRing1",
    "[10.009569] <Surreal Engine>: call FindGoal returned <unnamed DeusExGoal>",
    "[11.009144] <Surreal Engine>: call GetNote returned <unnamed DeusExNote>",
    "[14.997557] <Surreal Engine>: call ClientTravel returned nothing",
    "[14.997557] <Surreal Engine>: Client travel to 02_NYC_BatteryPark?Name=Player",
    "[14.997557] <Surreal Engine>: Skipping travel transfer. No travel data",
    "[20.009569] <Surreal Engine>: call FindGoal returned None",
    "[21.009144] <Surreal Engine>: call GetNote returned <unnamed DeusExNote>",
]


def classes(window):
    return [item["class"] for item in window["items"]]


def item(cls, package="Save1", number=0):
    return {"package": package, "instance": f"{cls}{number}", "class": cls}


class ParseEngineLogTests(unittest.TestCase):
    def test_window_before_the_first_landmark_is_the_state_check(self):
        parsed = saveload.parse_engine_log(UG_SAVE)
        first = parsed["windows"][0]
        self.assertEqual(first["where"], "after-state")
        self.assertEqual(classes(first), ["StingerAmmo", "Stinger"])
        self.assertEqual(first["weapon"], "UnrealShare.Stinger")

    def test_save_line_gives_the_path_and_opens_a_window(self):
        parsed = saveload.parse_engine_log(UG_SAVE)
        self.assertEqual(parsed["saved_to"], "Unreal-226b-Unreal_Gold/Save/Save9.usa")
        self.assertEqual([w["where"] for w in parsed["windows"]], ["after-state", "after-save"])
        self.assertEqual(parsed["windows"][1]["items"], [])

    def test_loaded_line_gives_slot_and_map_and_opens_the_after_load_window(self):
        parsed = saveload.parse_engine_log(UG_LOAD_TRAVEL)
        self.assertEqual(parsed["loaded_slot"], 9)
        self.assertEqual(parsed["loaded_map"], "Vortex2")
        windows = parsed["windows"]
        self.assertEqual([w["where"] for w in windows],
                         ["after-state", "after-load", "after-travel", "after-save"])
        self.assertEqual(windows[1]["map"], "Vortex2")
        self.assertEqual(classes(windows[1]), ["StingerAmmo", "Stinger"])

    def test_missing_loaded_line_marks_the_load_as_not_made(self):
        parsed = saveload.parse_engine_log(UG_SAVE)
        self.assertEqual(classes(parsed["windows"][0]), ["StingerAmmo", "Stinger"])
        self.assertIsNone(parsed["loaded_slot"])
        self.assertIsNone(parsed["loaded_map"])
        self.assertNotIn("after-load", [w["where"] for w in parsed["windows"]])

    def test_travel_line_opens_the_after_travel_window_with_the_url_map(self):
        parsed = saveload.parse_engine_log(UG_LOAD_TRAVEL)
        self.assertEqual(len(parsed["windows"]), 4)
        travel = parsed["windows"][2]
        self.assertEqual(travel["where"], "after-travel")
        self.assertEqual(travel["map"], "Nyleve")
        self.assertEqual(classes(travel), ["DefaultAmmo", "DispersionPistol", "Stinger", "StingerAmmo"])

    def test_a_window_with_no_inventory_lines_has_no_items(self):
        lines = ["[1.0] <Surreal Engine>: Loaded game from slot 3: Dig",
                 "[2.0] GameInfo.Login: Login: Player"]
        parsed = saveload.parse_engine_log(lines)
        self.assertEqual([w["where"] for w in parsed["windows"]], ["after-state", "after-load"])
        self.assertEqual(parsed["windows"][1]["items"], [])
        self.assertEqual(parsed["loaded_slot"], 3)

    def test_inventory_line_splits_into_package_instance_and_class(self):
        parsed = saveload.parse_engine_log(DX_LOAD)
        self.assertEqual(len(parsed["windows"]), 3)
        self.assertEqual(parsed["windows"][1]["items"],
                         [{"package": "01_NYC_UNATCOHQ", "instance": "NanoKeyRing1", "class": "NanoKeyRing"}])

    def test_class_drops_every_trailing_digit_and_keeps_inner_ones(self):
        lines = ["[1.0] PlayerPawn.ShowInventory: Inv: Save1.UT_Eightball12",
                 "[1.0] PlayerPawn.ShowInventory: Inv: Save1.Flak3Shell0"]
        window = saveload.parse_engine_log(lines)["windows"][0]
        self.assertEqual(classes(window), ["UT_Eightball", "Flak3Shell"])

    def test_the_state_suffix_of_an_ut_line_is_dropped(self):
        windows = saveload.parse_engine_log(UT_TRAVEL)["windows"]
        self.assertEqual(len(windows), 3)
        window = windows[1]
        self.assertEqual(window["items"][1], {"package": "Save8", "instance": "miniammo0", "class": "miniammo"})
        self.assertEqual(classes(window), ["ImpactHammer", "miniammo", "enforcer"])

    def test_calls_are_listed_in_order_and_a_function_may_repeat(self):
        lines = ["[1.0] <Surreal Engine>: call FindGoal returned <unnamed DeusExGoal>",
                 "[2.0] <Surreal Engine>: call GetNote returned <unnamed DeusExNote>",
                 "[3.0] <Surreal Engine>: call FindGoal returned None",
                 "[4.0] <Surreal Engine>: call ClientTravel returned nothing"]
        calls = saveload.parse_engine_log(lines)["windows"][0]["calls"]
        self.assertEqual(calls, [{"function": "FindGoal", "value": "<unnamed DeusExGoal>"},
                                 {"function": "GetNote", "value": "<unnamed DeusExNote>"},
                                 {"function": "FindGoal", "value": "None"},
                                 {"function": "ClientTravel", "value": "nothing"}])

    def test_each_offered_item_is_listed_once_though_traced_under_two_prefixes(self):
        windows = saveload.parse_engine_log(UT_TRAVEL)["windows"]
        self.assertEqual(len(windows), 3)
        travel = windows[2]
        self.assertEqual(travel["where"], "after-travel")
        self.assertEqual(travel["offered"], ["ImpactHammer0", "enforcer0", "Miniammo0"])

    def test_offered_names_of_other_windows_do_not_count_for_a_travel(self):
        parsed = saveload.parse_engine_log(UT_TRAVEL)
        self.assertEqual(len(parsed["windows"]), 3)
        self.assertEqual(parsed["windows"][1]["offered"], [])
        self.assertEqual(parsed["windows"][2]["offered"], ["ImpactHammer0", "enforcer0", "Miniammo0"])

    def test_skipping_travel_transfer_marks_the_travel_window(self):
        windows = saveload.parse_engine_log(DX_LOAD)["windows"]
        self.assertEqual([w["where"] for w in windows], ["after-state", "after-load", "after-travel"])
        self.assertEqual([w["travel_skipped"] for w in windows], [False, False, True])
        self.assertEqual(classes(windows[1]), ["NanoKeyRing"])

    def test_every_skipping_line_is_kept_with_its_timestamp_removed(self):
        parsed = saveload.parse_engine_log(DX_LOAD)
        self.assertEqual(parsed["skip_lines"],
                         ["<Surreal Engine>: Skipping travel transfer. No travel data"])

    def test_an_unskipped_travel_has_no_skip_flag(self):
        windows = saveload.parse_engine_log(UT_TRAVEL)["windows"]
        self.assertEqual(len(windows), 3)
        self.assertEqual(classes(windows[2]), ["ImpactHammer", "enforcer"])
        self.assertFalse(windows[2]["travel_skipped"])

    def test_summon_lines_of_both_games_are_recorded(self):
        ug = saveload.parse_engine_log(UG_SAVE)
        self.assertEqual(ug["summoned"], ["UnrealShare.Stinger"])
        ut = saveload.parse_engine_log(["[1.0] TournamentPlayer.Summon: Fabricate Botpack.ShockRifle"])
        self.assertEqual(ut["summoned"], ["Botpack.ShockRifle"])

    def test_landmark_text_in_a_script_log_line_is_not_a_landmark(self):
        lines = ["[1.0] SomeActor.Log: Loaded game from slot 3: Dig",
                 "[2.0] PlayerPawn.ShowInventory: Inv: Save1.Stinger0"]
        parsed = saveload.parse_engine_log(lines)
        self.assertEqual(classes(parsed["windows"][0]), ["Stinger"])
        self.assertEqual(len(parsed["windows"]), 1)
        self.assertIsNone(parsed["loaded_slot"])

    def test_windows_are_landmarks_plus_one_and_noise_changes_nothing(self):
        rng = random.Random(20261019)
        landmarks = ["[1.0] <Surreal Engine>: Loaded game from slot 1: A",
                     "[1.0] <Surreal Engine>: Client travel to B?x=1",
                     "[1.0] <Surreal Engine>: Saved game to K/Save/Save1.usa"]
        noise = ["[0.0] GameInfo.Login: Login: Player", "[0.0] Pawn.Foo: bar", ""]
        for _ in range(40):
            quiet = []
            for _ in range(rng.randint(0, 3)):
                quiet.append(rng.choice(landmarks))
                quiet.append(f"[2.0] PlayerPawn.ShowInventory: Inv: Save1.Thing{rng.randint(0, 9)}")
            noisy = []
            for line in quiet:
                noisy.append(line)
                noisy.extend(rng.sample(noise, rng.randint(0, 2)))
            expected = 1 + sum(1 for line in quiet if "<Surreal Engine>" in line)
            a = saveload.parse_engine_log(quiet)
            b = saveload.parse_engine_log(noisy)
            self.assertEqual(len(a["windows"]), expected)
            self.assertEqual(a, b)


class ParseSmokeExecTests(unittest.TestCase):
    def test_failed_and_skipped_exec_lines_are_listed(self):
        lines = ["GAME Unreal 226b", "EXEC 5 Fire", "EXEC_FAILED 7 call Nope 1",
                 "EXEC_SKIPPED 40 savegame 1", "SUMMARY script_errors=0"]
        got = saveload.parse_smoke_exec(lines)
        self.assertEqual(got, {"failed": ["7 call Nope 1"], "skipped": ["40 savegame 1"]})

    def test_a_clean_log_lists_nothing_for_both(self):
        got = saveload.parse_smoke_exec(["GAME Unreal 226b", "EXEC 5 Fire", "EXEC 6 savegame 1"])
        self.assertEqual(got, {"failed": [], "skipped": []})


class CompareTests(unittest.TestCase):
    def test_a_load_and_a_travel_compare_equal_by_class_with_a_changed_instance_number(self):
        before = [item("NanoKeyRing", "01_NYC_UNATCOIsland", 1)]
        after = [item("NanoKeyRing", "01_NYC_UNATCOHQ", 5)]
        self.assertEqual(saveload.compare(before, after),
                         {"kept": ["NanoKeyRing"], "lost": [], "gained": []})

    def test_the_package_prefix_is_ignored(self):
        before = [item("Stinger", "Save9"), item("StingerAmmo", "Save9")]
        after = [item("Stinger", "NyLeve"), item("StingerAmmo", "NyLeve")]
        self.assertEqual(saveload.compare(before, after)["kept"], ["Stinger", "StingerAmmo"])

    def test_class_names_compare_without_case(self):
        before = [item("miniammo", "Save8")]
        after = [item("Miniammo", "DM-Turbine")]
        got = saveload.compare(before, after)
        self.assertEqual(got["kept"], ["miniammo"])
        self.assertEqual(got["lost"], [])
        self.assertEqual(got["gained"], [])

    def test_what_is_missing_after_is_lost_and_what_is_new_is_gained(self):
        before = [item("ShockRifle"), item("enforcer")]
        after = [item("enforcer"), item("ImpactHammer")]
        self.assertEqual(saveload.compare(before, after),
                         {"kept": ["enforcer"], "lost": ["ShockRifle"], "gained": ["ImpactHammer"]})

    def test_an_empty_before_gains_everything_and_an_empty_after_loses_everything(self):
        self.assertEqual(saveload.compare([], [item("Stinger")]),
                         {"kept": [], "lost": [], "gained": ["Stinger"]})
        self.assertEqual(saveload.compare([item("Stinger")], []),
                         {"kept": [], "lost": ["Stinger"], "gained": []})

    def test_a_repeated_class_is_counted_not_collapsed(self):
        before = [item("Rocket", number=0), item("Rocket", number=1)]
        after = [item("Rocket", number=3)]
        self.assertEqual(saveload.compare(before, after),
                         {"kept": ["Rocket"], "lost": ["Rocket"], "gained": []})

    def test_every_item_lands_in_exactly_one_bucket_per_side(self):
        rng = random.Random(7)
        pool = ["Stinger", "stinger", "ShockRifle", "Miniammo", "enforcer", "NanoKeyRing"]
        for _ in range(60):
            before = [item(rng.choice(pool), number=rng.randint(0, 9)) for _ in range(rng.randint(0, 6))]
            after = [item(rng.choice(pool), number=rng.randint(0, 9)) for _ in range(rng.randint(0, 6))]
            got = saveload.compare(before, after)
            self.assertEqual(len(got["kept"]) + len(got["lost"]), len(before))
            self.assertEqual(len(got["kept"]) + len(got["gained"]), len(after))
            same = saveload.compare(before, list(reversed(before)))
            self.assertEqual((len(same["kept"]), same["lost"], same["gained"]), (len(before), [], []))


class CallStatusTests(unittest.TestCase):
    def test_returned_none_marks_the_goal_lost(self):
        calls = [{"function": "FindGoal", "value": "None"}]
        self.assertEqual(saveload.call_status(calls, "FindGoal"), "lost")

    def test_an_unnamed_object_marks_the_goal_found(self):
        calls = [{"function": "FindGoal", "value": "<unnamed DeusExGoal>"}]
        self.assertEqual(saveload.call_status(calls, "FindGoal"), "found")

    def test_the_last_call_of_a_function_decides(self):
        calls = [{"function": "FindGoal", "value": "<unnamed DeusExGoal>"},
                 {"function": "GetNote", "value": "None"},
                 {"function": "FindGoal", "value": "None"}]
        self.assertEqual(saveload.call_status(calls, "FindGoal"), "lost")
        self.assertEqual(saveload.call_status(calls, "GetNote"), "lost")

    def test_a_function_never_called_has_no_status(self):
        calls = [{"function": "FindGoal", "value": "<unnamed DeusExGoal>"}]
        self.assertEqual(saveload.call_status(calls, "findgoal"), "found")
        self.assertIsNone(saveload.call_status(calls, "GetNote"))


class BuildExecTests(unittest.TestCase):
    def test_schedule_is_in_time_order_with_the_given_gaps(self):
        plan = [(5, "Fire"), (2, "IAmTheOne"), (1, "summon Botpack.ShockRifle"),
                (2, "hold axis aForward 300 1.5")]
        self.assertEqual(saveload.build_exec(plan),
                         "5:Fire;7:IAmTheOne;8:summon Botpack.ShockRifle;10:hold axis aForward 300 1.5")

    def test_a_zero_gap_keeps_the_written_order_at_the_same_time(self):
        self.assertEqual(saveload.build_exec([(5, "a"), (0, "b")]), "5:a;5:b")

    def test_fractional_gaps_are_written_as_decimals(self):
        self.assertEqual(saveload.build_exec([(5, "a"), (0.5, "b")]), "5:a;5.5:b")

    def test_the_text_reads_back_to_the_plan(self):
        rng = random.Random(3)
        for _ in range(40):
            plan = [(rng.choice([0, 1, 2, 5, 10, 0.5]), f"cmd{n} arg:{n}") for n in range(rng.randint(1, 8))]
            text = saveload.build_exec(plan)
            self.assertNotEqual(text, "")
            items = [part.partition(":") for part in text.split(";")]
            self.assertEqual([c for _, _, c in items], [c for _, c in plan])
            times = [float(t) for t, _, _ in items]
            self.assertEqual(times, sorted(times))
            total = 0.0
            for (gap, _), t in zip(plan, times):
                total += gap
                self.assertAlmostEqual(t, total)

    def test_a_command_with_a_semicolon_is_refused(self):
        with self.assertRaisesRegex(ValueError, "semicolon"):
            saveload.build_exec([(5, "summon A;summon B")])

    def test_a_negative_gap_is_refused(self):
        with self.assertRaisesRegex(ValueError, "negative"):
            saveload.build_exec([(5, "a"), (-1, "b")])


def schedule(chain, index):
    """The step's (time, command) items, read back from the text the driver passes."""
    text = saveload.build_exec(saveload.step_plan(chain, index))
    return [(float(t), c) for t, _, c in (part.partition(":") for part in text.split(";"))]


class ChainTests(unittest.TestCase):
    def chain(self, name):
        self.assertIn(name, saveload.CHAINS)
        return saveload.CHAINS[name]

    def test_there_are_three_chains_of_three_maps_each(self):
        self.assertEqual(sorted(saveload.CHAINS), ["deusex", "unreal", "ut"])
        self.assertEqual(saveload.CHAINS["unreal"].maps, ("Vortex2", "Nyleve", "Dig"))
        self.assertEqual(saveload.CHAINS["ut"].maps, ("DM-Deck16][", "DM-Turbine", "DM-Codex"))
        self.assertEqual(saveload.CHAINS["deusex"].maps,
                         ("01_NYC_UNATCOIsland", "01_NYC_UNATCOHQ", "02_NYC_BatteryPark"))

    def test_a_chain_is_four_steps_in_this_order(self):
        self.assertEqual(saveload.STEP_NAMES, ("save1", "load1-travel", "load2-travel", "load3"))
        self.assertEqual(saveload.STEP_WHERES,
                         (("after-state",), ("after-load", "after-travel"),
                          ("after-load", "after-travel"), ("after-load",)))

    def test_the_first_step_sets_state_checks_then_saves_slot_one(self):
        for name in ("unreal", "ut", "deusex"):
            chain = self.chain(name)
            commands = [c for _, c in schedule(chain, 0)]
            self.assertEqual(commands[-1], "savegame 1")
            self.assertEqual(commands[-1 - len(chain.check):-1], list(chain.check))
            self.assertEqual(commands[:len(chain.state)], [c for _, c in chain.state])

    def test_each_later_step_starts_by_loading_the_slot_the_step_before_saved(self):
        loads = {"unreal": "call ClientTravel ?load={} 0 0", "ut": "call ClientTravel ?load={} 0 0",
                 "deusex": "LoadGame {}"}
        for name, template in loads.items():
            chain = self.chain(name)
            for index in (1, 2, 3):
                self.assertEqual(schedule(chain, index)[0][1], template.format(index))

    def test_a_travelling_step_travels_to_the_next_map_and_saves_the_next_slot(self):
        expected = {"unreal": ("Nyleve", "Dig"), "ut": ("DM-Turbine?NumBots=0", "DM-Codex?NumBots=0"),
                    "deusex": ("01_NYC_UNATCOHQ", "02_NYC_BatteryPark")}
        for name, targets in expected.items():
            chain = self.chain(name)
            for index, target in zip((1, 2), targets):
                commands = [c for _, c in schedule(chain, index)]
                self.assertIn(f"call ClientTravel {target} 2 1", commands)
                self.assertEqual(commands[-1], f"savegame {index + 1}")

    def test_the_last_step_loads_and_checks_and_neither_travels_nor_saves(self):
        for name in ("unreal", "ut", "deusex"):
            chain = self.chain(name)
            commands = [c for _, c in schedule(chain, 3)]
            self.assertEqual(commands, ["call ClientTravel ?load=3 0 0" if name != "deusex" else "LoadGame 3"]
                             + list(chain.check))

    def test_ut_state_is_fire_cheats_summon_then_a_walk_and_every_travel_is_followed_by_fire(self):
        ut = self.chain("ut")
        self.assertEqual([c for _, c in schedule(ut, 0)][:4],
                         ["Fire", "IAmTheOne", "summon Botpack.ShockRifle", "hold axis aForward 300 1.5"])
        for index in (1, 2):
            commands = [c for _, c in schedule(ut, index)]
            travel = next(i for i, c in enumerate(commands) if c.startswith("call ClientTravel DM-"))
            self.assertEqual(commands[travel + 1], "Fire")
            self.assertEqual(commands[travel + 2], "ShowInventory")

    def test_ut_travelling_steps_trace_the_travel_acceptance_and_no_other_chain_does(self):
        self.assertEqual(self.chain("ut").trace, "Inventory.TravelPreAccept")
        self.assertIsNone(self.chain("unreal").trace)
        self.assertIsNone(self.chain("deusex").trace)

    def test_deus_ex_state_is_a_primary_goal_a_note_and_a_flag_and_the_check_reads_goal_and_note(self):
        dx = self.chain("deusex")
        self.assertEqual([c for _, c in schedule(dx, 0)][:3],
                         ["call AddGoal T3Goal 1", "NoteAdd T3Note", "call SetBoolFlagFromString T3Flag 1"])
        self.assertEqual(list(dx.check), ["ShowInventory", "call FindGoal T3Goal", "call GetNote None"])
        self.assertEqual(sorted(dx.check_calls), ["FindGoal", "GetNote"])

    def test_unreal_state_is_a_summoned_stinger_and_a_walk(self):
        ug = self.chain("unreal")
        self.assertEqual([c for _, c in schedule(ug, 0)][:2],
                         ["summon UnrealShare.Stinger", "hold axis aForward 300 1.5"])
        self.assertEqual(list(ug.check), ["ShowInventory"])

    def test_five_seconds_separate_a_load_or_travel_from_its_checks_and_ten_the_last_check_from_a_save(self):
        for name in ("unreal", "ut", "deusex"):
            chain = self.chain(name)
            for index in range(4):
                items = schedule(chain, index)
                check_at = [i for i, (_, c) in enumerate(items) if c == chain.check[0]]
                self.assertEqual(len(check_at), 2 if index in (1, 2) else 1)
                for i in check_at:
                    self.assertGreaterEqual(items[i][0] - items[i - 1][0], 5)
                if index < 3:
                    last_check = max(t for t, c in items if c in chain.check)
                    self.assertEqual(items[-1][0] - last_check, 10)

    def test_every_step_is_scheduled_well_inside_the_default_run_time(self):
        for name in ("unreal", "ut", "deusex"):
            for index in range(4):
                self.assertLessEqual(schedule(self.chain(name), index)[-1][0], saveload.DEFAULT_SECONDS - 20)


def make_info(**overrides):
    info = {"summary": True, "script_errors": 0, "missing_natives": 0,
            "exceptions": 0, "script_warnings": 0, "fatal_signal": False,
            "maps_loaded": ["Vortex2"], "fps_avg": 60.0}
    info.update(overrides)
    return info


class StepVerdictTests(unittest.TestCase):
    def test_a_clean_step_passes(self):
        self.assertEqual(saveload.step_verdict(make_info(), 0, False, "Vortex2"), "pass")

    def test_the_expected_map_matches_without_case(self):
        info = make_info(maps_loaded=["Vortex2", "NyLeve"])
        self.assertEqual(saveload.step_verdict(info, 0, False, "Nyleve"), "pass")

    def test_a_step_that_never_reached_its_last_map_is_idle(self):
        info = make_info(maps_loaded=["Vortex2"])
        self.assertEqual(saveload.step_verdict(info, 0, False, "Nyleve"), "idle")

    def test_a_crash_and_a_timeout_keep_the_sweep_verdicts(self):
        self.assertEqual(saveload.step_verdict(make_info(fatal_signal=True), -11, False, "Vortex2"), "crash")
        self.assertEqual(saveload.step_verdict(make_info(), -9, True, "Vortex2"), "timeout")


def check(where, map_name, class_names, calls=()):
    return {"where": where, "map": map_name,
            "items": [item(c) for c in class_names],
            "calls": [{"function": f, "value": v} for f, v in calls]}


NAMES = ("save1", "load1-travel", "load2-travel", "load3")
WHERES = (("after-state",), ("after-load", "after-travel"),
          ("after-load", "after-travel"), ("after-load",))


def make_steps(with_checks=None):
    """Four steps whose checks are given as {(step, where): (map, classes, calls)}."""
    spec = with_checks or {}
    steps = []
    for index, (name, wheres) in enumerate(zip(NAMES, WHERES)):
        checks = [check(w, *spec[(name, w)]) for w in wheres if (name, w) in spec]
        steps.append({"step": name, "exit_code": 0, "verdict": "pass", "maps_loaded": [],
                      "loaded_slot": index if index else None,
                      "saved_to": f"Game/Save/Save{index + 1}" if index < 3 else None,
                      "checks": checks, "offered": [], "travel_skipped": False,
                      "exec_failed": [], "exec_skipped": []})
    return steps


UG_CHECKS = {
    ("save1", "after-state"): ("Vortex2", ["Stinger", "StingerAmmo"], []),
    ("load1-travel", "after-load"): ("Vortex2", ["Stinger", "StingerAmmo"], []),
    ("load1-travel", "after-travel"): ("Nyleve", ["Stinger", "StingerAmmo", "DispersionPistol"], []),
    ("load2-travel", "after-load"): ("Nyleve", ["Stinger", "StingerAmmo", "DispersionPistol"], []),
    ("load2-travel", "after-travel"): ("Dig", ["Stinger", "StingerAmmo", "DispersionPistol"], []),
    ("load3", "after-load"): ("Dig", ["Stinger", "StingerAmmo", "DispersionPistol"], []),
}


class BuildHopsTests(unittest.TestCase):
    def test_four_steps_with_every_check_give_five_hops_alternating_load_and_travel(self):
        hops = saveload.build_hops(make_steps(with_checks=UG_CHECKS))
        self.assertEqual(len(hops), 5)
        self.assertEqual([h["by"] for h in hops], ["load", "travel", "load", "travel", "load"])
        self.assertEqual(hops[0]["from"], "save1/after-state")
        self.assertEqual(hops[0]["to"], "load1-travel/after-load")
        self.assertEqual(hops[4]["from"], "load2-travel/after-travel")
        self.assertEqual(hops[4]["to"], "load3/after-load")

    def test_a_hop_names_both_maps_and_what_was_kept_lost_and_gained(self):
        hops = saveload.build_hops(make_steps(with_checks=UG_CHECKS))
        self.assertEqual(len(hops), 5)
        travel = hops[1]
        self.assertEqual((travel["map_from"], travel["map_to"]), ("Vortex2", "Nyleve"))
        self.assertEqual(travel["kept"], ["Stinger", "StingerAmmo"])
        self.assertEqual(travel["lost"], [])
        self.assertEqual(travel["gained"], ["DispersionPistol"])

    def test_a_missing_check_removes_the_hops_it_ends_and_starts(self):
        spec = dict(UG_CHECKS)
        del spec[("load1-travel", "after-travel")]
        hops = saveload.build_hops(make_steps(with_checks=spec))
        self.assertEqual([h["to"] for h in hops],
                         ["load1-travel/after-load", "load2-travel/after-travel", "load3/after-load"])

    def test_no_checks_give_no_hops_and_one_check_gives_none(self):
        self.assertEqual(saveload.build_hops(make_steps()), [])
        only = {("save1", "after-state"): UG_CHECKS[("save1", "after-state")]}
        self.assertEqual(saveload.build_hops(make_steps(with_checks=only)), [])
        self.assertEqual(len(saveload.build_hops(make_steps(with_checks=UG_CHECKS))), 5)

    def test_the_goal_and_note_at_a_hops_end_are_found_or_lost(self):
        found = [("FindGoal", "<unnamed DeusExGoal>"), ("GetNote", "<unnamed DeusExNote>")]
        gone = [("FindGoal", "None"), ("GetNote", "<unnamed DeusExNote>")]
        spec = {("save1", "after-state"): ("A", ["NanoKeyRing"], found),
                ("load1-travel", "after-load"): ("A", ["NanoKeyRing"], found),
                ("load1-travel", "after-travel"): ("B", ["NanoKeyRing"], gone)}
        hops = saveload.build_hops(make_steps(with_checks=spec))
        self.assertEqual(len(hops), 2)
        self.assertEqual((hops[0]["goal"], hops[0]["note"]), ("found", "found"))
        self.assertEqual((hops[1]["goal"], hops[1]["note"]), ("lost", "found"))

    def test_goal_and_note_are_absent_where_no_such_call_was_made(self):
        hops = saveload.build_hops(make_steps(with_checks=UG_CHECKS))
        self.assertEqual(len(hops), 5)
        self.assertNotIn("goal", hops[0])
        self.assertNotIn("note", hops[0])

    def test_a_travel_hop_carries_the_steps_offered_list_and_skip_flag(self):
        steps = make_steps(with_checks=UG_CHECKS)
        steps[1]["offered"] = ["enforcer0", "Miniammo0"]
        steps[1]["travel_skipped"] = True
        hops = saveload.build_hops(steps)
        self.assertEqual(len(hops), 5)
        self.assertEqual(hops[1]["offered"], ["enforcer0", "Miniammo0"])
        self.assertTrue(hops[1]["travel_skipped"])
        self.assertNotIn("offered", hops[0])


def full_report(game, spec, **step_overrides):
    steps = make_steps(spec)
    for step in steps:
        step.update(step_overrides)
    return {"game": "<game folder>", "chain": game, "seconds": 60, "steps": steps,
            "hops": saveload.build_hops(steps)}


DX_FOUND = [("FindGoal", "<unnamed DeusExGoal>"), ("GetNote", "<unnamed DeusExNote>")]
DX_CHECKS = {k: (m, ["NanoKeyRing"], DX_FOUND) for k, m in {
    ("save1", "after-state"): "01_NYC_UNATCOIsland",
    ("load1-travel", "after-load"): "01_NYC_UNATCOIsland",
    ("load1-travel", "after-travel"): "01_NYC_UNATCOHQ",
    ("load2-travel", "after-load"): "01_NYC_UNATCOHQ",
    ("load2-travel", "after-travel"): "02_NYC_BatteryPark",
    ("load3", "after-load"): "02_NYC_BatteryPark"}.items()}
UT_CHECKS = {
    ("save1", "after-state"): ("DM-Deck16][", ["enforcer", "ShockRifle"], []),
    ("load1-travel", "after-load"): ("DM-Deck16][", ["enforcer", "ShockRifle"], []),
    ("load1-travel", "after-travel"): ("DM-Turbine", ["enforcer"], []),
    ("load2-travel", "after-load"): ("DM-Turbine", ["enforcer"], []),
    ("load2-travel", "after-travel"): ("DM-Codex", ["enforcer"], []),
    ("load3", "after-load"): ("DM-Codex", ["enforcer"], []),
}


class RenderTests(unittest.TestCase):
    def test_one_row_per_hop(self):
        for game, spec in (("unreal", UG_CHECKS), ("deusex", DX_CHECKS), ("ut", UT_CHECKS)):
            report = full_report(game, spec)
            text = saveload.render_markdown(report)
            self.assertNotEqual(text, "")
            self.assertEqual(len(report["hops"]), 5)
            for hop in report["hops"]:
                rows = [line for line in text.splitlines()
                        if line.startswith("|") and hop["from"] in line and hop["to"] in line]
                self.assertEqual(len(rows), 1, (game, hop["from"], hop["to"]))

    def test_every_check_lists_its_inventory_by_class(self):
        text = saveload.render_markdown(full_report("unreal", UG_CHECKS))
        rows = [line for line in text.splitlines() if line.startswith("| load1-travel/after-travel |")]
        self.assertEqual(len(rows), 1)
        self.assertIn("DispersionPistol", rows[0])
        self.assertIn("Nyleve", rows[0])

    def test_a_complete_chain_with_nothing_lost_says_state_and_inventory_survive_three_maps(self):
        sentence = saveload.verdict_sentence(full_report("unreal", UG_CHECKS))
        self.assertIn("state and inventory survive three maps", sentence)
        self.assertIn("state and inventory survive three maps", saveload.render_markdown(full_report("unreal", UG_CHECKS)))

    def test_deus_ex_adds_the_goal_and_note_to_the_survival_sentence(self):
        sentence = saveload.verdict_sentence(full_report("deusex", DX_CHECKS))
        self.assertIn("state, inventory and the Deus Ex goal and note survive three maps", sentence)

    def test_a_lost_item_is_named_with_its_hop_and_the_survival_claim_is_withheld(self):
        spec = dict(UG_CHECKS)
        spec[("load2-travel", "after-travel")] = ("Dig", ["DispersionPistol"], [])
        spec[("load3", "after-load")] = ("Dig", ["DispersionPistol"], [])
        sentence = saveload.verdict_sentence(full_report("unreal", spec))
        self.assertNotIn("survive three maps", sentence)
        self.assertIn("load2-travel/after-load to load2-travel/after-travel", sentence)
        self.assertIn("Stinger", sentence)

    def test_a_lost_goal_is_named_with_its_hop_and_the_survival_claim_is_withheld(self):
        spec = dict(DX_CHECKS)
        spec[("load2-travel", "after-travel")] = ("02_NYC_BatteryPark", ["NanoKeyRing"],
                                                  [("FindGoal", "None"), ("GetNote", "<unnamed DeusExNote>")])
        sentence = saveload.verdict_sentence(full_report("deusex", spec))
        self.assertNotIn("survive three maps", sentence)
        self.assertIn("goal", sentence)
        self.assertIn("load2-travel/after-load to load2-travel/after-travel", sentence)

    def test_a_load_that_arrives_on_another_map_withholds_the_claim_and_names_the_hop(self):
        spec = dict(UG_CHECKS)
        spec[("load2-travel", "after-load")] = ("Dig", ["Stinger", "StingerAmmo", "DispersionPistol"], [])
        sentence = saveload.verdict_sentence(full_report("unreal", spec))
        self.assertNotIn("survive three maps", sentence)
        self.assertIn("load1-travel/after-travel to load2-travel/after-load", sentence)
        self.assertIn("Nyleve", sentence)
        self.assertIn("Dig", sentence)

    def test_the_same_map_in_another_case_is_not_a_mismatch(self):
        spec = dict(UG_CHECKS)
        spec[("load2-travel", "after-load")] = ("NyLeve", ["Stinger", "StingerAmmo", "DispersionPistol"], [])
        sentence = saveload.verdict_sentence(full_report("unreal", spec))
        self.assertIn("state and inventory survive three maps", sentence)

    def test_ut_loads_restore_and_travels_discard_by_game_rule(self):
        report = full_report("ut", UT_CHECKS)
        sentence = saveload.verdict_sentence(report)
        self.assertIn("loads restore what was saved", sentence)
        self.assertIn("by game rule", sentence)
        self.assertIn("ShockRifle", sentence)
        self.assertNotIn("survive three maps", sentence)

    def test_an_ut_load_that_lost_an_item_is_not_called_a_restore(self):
        spec = dict(UT_CHECKS)
        spec[("load1-travel", "after-load")] = ("DM-Deck16][", ["enforcer"], [])
        sentence = saveload.verdict_sentence(full_report("ut", spec))
        self.assertNotIn("loads restore what was saved", sentence)
        self.assertIn("save1/after-state to load1-travel/after-load", sentence)
        self.assertIn("ShockRifle", sentence)

    def test_a_chain_that_stopped_says_where_and_how(self):
        report = full_report("unreal", {k: v for k, v in UG_CHECKS.items() if k[0] == "save1"})
        report["steps"] = report["steps"][:1]
        self.assertEqual(len(report["steps"]), 1)
        report["steps"][0]["verdict"] = "crash"
        sentence = saveload.verdict_sentence(report)
        self.assertTrue(sentence.startswith("Incomplete"), sentence)
        self.assertIn("save1", sentence)
        self.assertIn("crash", sentence)

    def test_the_limits_of_each_claim_are_stated(self):
        dx = saveload.render_markdown(full_report("deusex", DX_CHECKS))
        self.assertIn("key ring", dx)
        self.assertIn("flag", dx)
        ut = saveload.render_markdown(full_report("ut", UT_CHECKS))
        self.assertIn("by game rule", ut)
        self.assertIn("TravelPreAccept", ut)

    def test_the_ut_table_has_an_offered_column_and_the_deus_ex_table_a_goal_and_note_column(self):
        ut_report = full_report("ut", UT_CHECKS)
        self.assertEqual(len(ut_report["hops"]), 5)
        ut_report["hops"][1]["offered"] = ["enforcer0", "Miniammo0"]
        ut = saveload.render_markdown(ut_report)
        header = next(line for line in ut.splitlines() if line.startswith("| Hop |"))
        self.assertIn("Offered", header)
        self.assertIn("enforcer0, Miniammo0", ut)
        dx = saveload.render_markdown(full_report("deusex", DX_CHECKS))
        header = next(line for line in dx.splitlines() if line.startswith("| Hop |"))
        self.assertIn("Goal", header)
        self.assertIn("Note", header)
        self.assertIn("found", dx)


class StepProblemTests(unittest.TestCase):
    def steps(self):
        return make_steps(UG_CHECKS)

    def test_a_clean_step_has_no_problem_at_any_position(self):
        steps = self.steps()
        self.assertEqual([saveload.step_problem(s, i) for i, s in enumerate(steps)], [None] * 4)

    def test_a_verdict_other_than_pass_is_the_problem(self):
        steps = self.steps()
        steps[1]["verdict"] = "crash"
        problem = saveload.step_problem(steps[1], 1)
        self.assertIn("load1-travel", problem)
        self.assertIn("crash", problem)

    def test_a_failed_or_skipped_command_is_a_problem(self):
        steps = self.steps()
        steps[0]["exec_failed"] = ["7 call Nope 1"]
        self.assertIn("failed", saveload.step_problem(steps[0], 0))
        steps[2]["exec_skipped"] = ["40 savegame 3"]
        self.assertIn("skipped", saveload.step_problem(steps[2], 2))

    def test_a_load_that_was_not_made_is_a_problem_from_the_second_step_on(self):
        steps = self.steps()
        steps[2]["loaded_slot"] = None
        self.assertIn("load", saveload.step_problem(steps[2], 2))
        self.assertIsNone(saveload.step_problem(steps[0], 0))

    def test_a_load_of_another_slot_is_a_problem(self):
        steps = self.steps()
        steps[1]["loaded_slot"] = 3
        problem = saveload.step_problem(steps[1], 1)
        self.assertIn("slot 1", problem)
        self.assertIn("slot 3", problem)

    def test_a_missing_check_window_is_named(self):
        steps = self.steps()
        steps[1]["checks"] = steps[1]["checks"][:1]
        self.assertIn("after-travel", saveload.step_problem(steps[1], 1))

    def test_a_save_that_was_not_made_is_a_problem_except_on_the_last_step(self):
        steps = self.steps()
        steps[0]["saved_to"] = None
        self.assertIn("save", saveload.step_problem(steps[0], 0))
        self.assertIsNone(saveload.step_problem(steps[3], 3))


class AssembleChecksTests(unittest.TestCase):
    def chain(self, name):
        self.assertIn(name, saveload.CHAINS)
        return saveload.CHAINS[name]

    def test_the_first_step_keeps_the_state_window_and_names_the_start_map(self):
        parsed = saveload.parse_engine_log(UG_SAVE)
        got = saveload.assemble_checks(parsed, 0, self.chain("unreal"))
        self.assertEqual([c["where"] for c in got["checks"]], ["after-state"])
        self.assertEqual(got["checks"][0]["map"], "Vortex2")
        self.assertEqual([i["class"] for i in got["checks"][0]["items"]], ["StingerAmmo", "Stinger"])
        self.assertEqual(got["checks"][0]["weapon"], "UnrealShare.Stinger")

    def test_a_travelling_step_keeps_the_load_and_travel_windows_only(self):
        parsed = saveload.parse_engine_log(UG_LOAD_TRAVEL)
        got = saveload.assemble_checks(parsed, 1, self.chain("unreal"))
        self.assertEqual([(c["where"], c["map"]) for c in got["checks"]],
                         [("after-load", "Vortex2"), ("after-travel", "Nyleve")])

    def test_a_missing_load_line_leaves_no_after_load_check(self):
        parsed = saveload.parse_engine_log(UG_SAVE)
        got = saveload.assemble_checks(parsed, 1, self.chain("unreal"))
        self.assertEqual(got["checks"], [])
        self.assertEqual(got["offered"], [])

    def test_calls_are_cut_to_the_chains_check_functions(self):
        parsed = saveload.parse_engine_log(DX_LOAD)
        got = saveload.assemble_checks(parsed, 1, self.chain("deusex"))
        self.assertEqual([c["where"] for c in got["checks"]], ["after-load", "after-travel"])
        self.assertEqual(got["checks"][0]["calls"],
                         [{"function": "FindGoal", "value": "<unnamed DeusExGoal>"},
                          {"function": "GetNote", "value": "<unnamed DeusExNote>"}])
        self.assertEqual(got["checks"][1]["calls"][0], {"function": "FindGoal", "value": "None"})

    def test_offered_names_and_the_skip_flag_come_from_the_travel_window(self):
        ut = saveload.assemble_checks(saveload.parse_engine_log(UT_TRAVEL), 1, self.chain("ut"))
        self.assertEqual(ut["offered"], ["ImpactHammer0", "enforcer0", "Miniammo0"])
        self.assertFalse(ut["travel_skipped"])
        dx = saveload.assemble_checks(saveload.parse_engine_log(DX_LOAD), 1, self.chain("deusex"))
        self.assertTrue(dx["travel_skipped"])
        self.assertEqual(dx["offered"], [])


if __name__ == "__main__":
    unittest.main()
