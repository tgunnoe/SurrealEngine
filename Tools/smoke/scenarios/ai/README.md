# Scripted pawn runs

These runs drive stock maps of Unreal Gold 226b, Unreal Tournament 436 and Deus Ex 1112fm to show what three changes to the scripted pawns do: the body pitch cap, the `LongFall` event, and the Return to Na Pali Predator's path node iterator, which was implemented and then withdrawn.

- **Body pitch.** `UPawn::TickRotating` bounds a pawn's body pitch by the pitch of its rotation rate. The upward branch was inverted: an upward pitch below the rate was lifted to the rate, and a larger one was not capped. It now caps the upward pitch at the rate, as the stock scripts do for a player's body. No harness command prints a rotation, so the unit tests close this (`tests/ai/BodyPitchTests.cpp`) and no run claims it.
- **`LongFall`.** `WaitForLanding` never raised the event the stock pawn scripts of all three games handle. It now raises it once, by name, when a fall inside `WaitForLanding` has lasted 2.0 seconds of engine time. The threshold is designed from the stock `FallingState`, which sleeps 2.0 seconds before its `LongFall` label in all three games; it is not recovered from the original game.
- **Path node iterator.** The Predator flees with `PathNodeIterator` (13 calls, all in its `Hiding` state). The six natives with callers were implemented, with semantics designed from the stock class declaration and the Predator's calls, and then withdrawn: the natives answer None and log as before, `NodePath` is the stock fixed array of 64, and `CheckUPak`, which the stock class does not have, is cut. The Predator therefore attacks instead of fleeing, as before. This README never claims that it flees.

## Why the iterator was withdrawn

With the iterator implemented, a Predator followed its path to a hiding point and, standing there in its enemy's sight, ran `PickNextSpot`, `ReachedHome` and `PickDestination` back to the `RunAway` label with no latent call in between. The engine never left that state code. Two of six Nevec runs on that build did not exit: one was killed by the run template after its time limit, and in the other the stall watcher below aborted the engine after 48 seconds without log growth. The core of the second held the trace of 20,000 calls (10,029 `PickNextSpot` and 9,730 `PickDestination` of one Predator, no `ChangeDestination`, no further `BuildPath`) and 140,026 lines of the VM's "Too many VM instructions executed in a single tick" message, whose guard logs and does not stop the run.

No iterator semantics prevents the cycle. When no hidden point qualifies, `PickDestination` keeps its home whatever the iterator answers. Otherwise its exit needs "no current node" on arrival, which an iterator that does not know its pawn's position cannot tell apart from the moment after a re-plan, when the script needs that node. The semantics were designed from the callers, and the callers do not constrain them enough.

## Recipe

Every run is an unattended smoke run with no `--user-dir`, so the engine writes nothing into the game folder:

```
SurrealEngine --autostart --exit-after=<seconds> --smoke-log=<log> --url=<map> \
    --exec='<items>' --trace-script=<entries> "<game folder>"
```

Tools/smoke/README.md describes the flags, `place` and `call`. What these runs add:

- The engine log (written when the engine exits) holds one `trace:` line per traced call. A `Super` call is traced too and prints the same text, so a function that calls its `Super` doubles its line; `Landed(` and `LongFall()` can print twice at one stamp (the state's function and the global one), and a pair counts once.
- The `--exec` seconds are wall seconds since the first login; the engine log's stamps are level time. A long frame (the smoke log shows an `FPS` line under 1) makes the two drift apart by seconds, and the level clock then runs behind the `--exec` clock. Every interval below is read from the engine log's stamps. A run with such a stall between the placement and the landing is void.
- The physics step is the frame time (about 1 ms on the host used), so no threshold is a count of ticks.
- A stall watcher wrapped the run command from the second half of the Nevec runs on: when the smoke log does not grow for 45 seconds it sends SIGABRT to the engine, which leaves a core for a backtrace instead of waiting for the template's kill.
- Probes: `call LineOfSightTo name:<Object>` or `class:<PlayerClass>` logs `call: <form> resolved to <Name> at (x, y, z)`. The Unreal Gold player class is `UPakFemaleOne`.

### Predator counts

Per run, `<engine>` is the engine log and `<log>` the smoke log:

- `reached`: `grep -a -c 'Predator Hiding: PickDestination(' <engine>`
- `stubs`: `grep -a -c '^STUB PathNodeIterator' <log>`
- `buildpath errors`: `grep -a -c '^SCRIPT_ERROR.*PathNodeIterator' <log>`
- `pickdest warnings`: `grep -a -c '^SCRIPT_WARNING Accessed None in Predator[.]PickDestination' <log>`
- `buildpath`, `getnext`: the counts of `: BuildPath(` and `: GetNext(` in `<engine>`
- `follow-on`: the counts of `Predator Hiding: ChangeDestination(` and `Predator Hiding: PickNextSpot(` added
- `verdict`: the sweep runner's `classify` over `parse_log`.

Follow-on counts depend on the frame rate (about 900 frames a second here): they are reported and never judged. A run capped by the trace's 20,000 calls has lower-bound counts.

Both maps are in `Maps/UPak` of the Unreal Gold folder; the engine finds them by bare name. Trace: `Predator.PickDestination,Predator.ChangeDestination,Predator.PickNextSpot,PathNodeIterator.BuildPath,PathNodeIterator.GetNext`, 90 seconds each, interleaved between the two binaries.

- Nevec string (idle player): a probe `call LineOfSightTo name:Predator<N>` every 5 seconds from 2 seconds, for `N` 4 to 7. The four Predators reach `Hiding` within a second without any input.
- NagomiSun string (found by two discovery runs on the previous binary): the same probes for `N` 0 to 2, and `6:place 300 1500 -2530 90;7:Summon UnrealShare.DispersionPistol;8:hold axis aForward 300 0.5;10:SwitchWeapon 1`, then `Fire` every 0.4 seconds from 11 to 30. The player is placed near the Predators, picks up the pistol and shoots.

## Predator runs on the first build of the iterator

`Previous binary` is the build before the series; `Fixed binary` is the build with the iterator implemented, which was later withdrawn. Columns: reached / stubs / buildpath errors / script errors / pickdest warnings / `BuildPath` / `GetNext` / follow-on.

### NagomiSun (stage-2 string)

| Run | Previous binary | Fixed binary |
| :--- | :--- | :--- |
| 1 | 0 / 0 / 0 / 0 / 0 / 0 / 0 / 0 | 0 / 0 / 0 / 0 / 0 / 0 / 0 / 0 |
| 2 | 0 / 0 / 0 / 0 / 0 / 0 / 0 / 0 | 0 / 0 / 0 / 0 / 0 / 0 / 0 / 0 |
| 3 | 0 / 0 / 0 / 0 / 0 / 0 / 0 / 0 | 0 / 0 / 0 / 0 / 0 / 0 / 0 / 0 |
| 4 | 0 / 0 / 0 / 0 / 0 / 0 / 0 / 0 | 1 / 0 / 0 / 0 / 0 / 1 / 24 / 0 |
| 5 | 1 / 3 / 0 / 0 / 0 / 1 / 0 / 0 | 0 / 0 / 0 / 0 / 0 / 0 / 0 / 0 |

Every run passed. Hiding was reached in 1 of 5 runs on each binary, so the map is short of the 3 of 5 that was asked for. The Predators that the player shoots die within seconds in the measured runs (their names stop resolving from about 7 seconds), while in the discovery runs a stalled level clock changed the timing. The probed Predator of the one fixed-binary run that reached `Hiding` shows 0.0 units of displacement (a single probe in the window). Only `PickDestination` ran on this map. No further NagomiSun run was made.

### Nevec (idle string)

| Run | Previous binary | Fixed binary |
| :--- | :--- | :--- |
| 1 | 5 / 4 / 0 / 0 / 0 / 5 / 0 / 0 | 100 / 0 / 0 / 0 / 0 / 29 / 140 / 4217 |
| 2 | 4 / 3 / 0 / 0 / 0 / 4 / 0 / 0 | killed by the template after its limit; rerun as 2b: 568 / 0 / 0 / 0 / 0 / 174 / 859 / 18399, capped at 40.5 s |
| 3 | 5 / 3 / 24 / 24 / 24 / 36 / 0 / 0 (verdict fail) | 402 / 0 / 0 / 0 / 0 / 180 / 724 / 18694, capped at 54.0 s |
| 4 | 4 / 3 / 0 / 0 / 0 / 4 / 0 / 0 | 177 / 0 / 0 / 0 / 0 / 78 / 298 / 9059 |
| 5 | 4 / 3 / 0 / 0 / 0 / 4 / 0 / 0 | hung: aborted by the stall watcher after 48 s, no engine log |

Run 3 of the previous binary reproduces the failure the committed sweep records: 24 script errors of the form "Accessed None" in the argument list of `PathNodeIterator.BuildPath` (the VM reads `home.Location` of None), each followed by an "Accessed None" warning in `Predator.PickDestination`. The other completed runs passed. The `Hiding` Predator (always `Predator5`, the first to enter) moved 813 to 1,004 units in the 5 seconds from its first probe on the previous binary and 389 to 541 units on the fixed one. Previous-binary counts of `reached` are the stub's few `PickDestination` calls; the fixed binary followed its paths (`GetNext` and follow-on above) and then hung twice in six runs, which is why the iterator was withdrawn.

## Predator runs on the final build (iterator withdrawn)

The same Nevec string and trace, twice (`node-6`, `node-7`): `reached` 4 and 4, `stubs` 3 and 3, 0 `GetNext(`, 0 follow-on, 0 "Too many VM instructions" lines, 0 script errors, verdict `pass`, exit 0. The `Hiding` Predator moved 909.5 and 1,073.4 units in 5 seconds, as on the previous binary. The natives answer None, the Predator falls back to attacking, and nothing loops.

## LongFall

A fall of more than 2.0 seconds inside `WaitForLanding` is reached with the player: `DuskFalls`, `place 5183 4593 -160` (open air above a floor at z -2960; a fall of 2.43 s) and `call WaitForLanding` 0.1 seconds later, trace `Pawn.WaitForLanding,Pawn.LongFall,Pawn.Landed`, `--exit-after=40`. Column discovery took three runs: (5183, 4593, -2942) is 18 units above the floor; the Toxic candidate was refused as rock; (5183, 4593, -160) fell 2.43 s from the placement to `Landed(`.

| Binary | Stamps | Result |
| :--- | :--- | :--- |
| Previous | `WaitForLanding()` 6.0986, `Landed(` 8.4298 | no `LongFall()` |
| Fixed | `WaitForLanding()` 14.0994, `LongFall()` 16.0999 (2.0005 s later), `Landed(` 16.4288 | exactly one `LongFall()`, before the landing |

Two earlier fixed-binary runs of this column are void and are listed so the run count is honest: in both, a long first frame made the level clock run behind the `--exec` clock, so the player was still in its waking state when `WaitForLanding` was called, and the state change that follows dropped the latent state before 2.0 seconds had passed. The accepted run uses the same choreography 8 seconds later. The `LongFall` of a monster was not run: no monster can be made to fall in a `WaitForLanding` on demand.

## Regression sample

Fifteen stock maps, 60 seconds each, on the final build, as the committed sweeps ran them (the three Unreal Tournament maps whose prefix the committed sweep mapped to a game ran as `AS-Frigate?game=Botpack.Assault`, `AS-Overlord?game=Botpack.Assault` and `CTF-Face?game=Botpack.CTFGame`). `matches` means the verdict and the counts of script errors, exceptions, missing natives and stub hits equal the committed row of the same map. Script warnings are not compared.

| Game | Map | Verdict | Script errors | Exceptions | Missing natives | Stub hits | Match | Script warnings (committed, run) |
| :--- | :--- | :--- | ---: | ---: | ---: | ---: | :--- | :--- |
| Unreal Gold | Vortex2 | pass | 0 | 0 | 0 | 0 | matches | 0, 0 |
| Unreal Gold | Dig | pass | 0 | 0 | 0 | 0 | matches | 10, 49 |
| Unreal Gold | NyLeve | pass | 0 | 0 | 0 | 0 | matches | 0, 6 |
| Unreal Gold | Chizra | pass | 0 | 0 | 0 | 0 | matches | 17, 18 |
| Unreal Gold | Harobed | pass | 0 | 0 | 0 | 0 | matches | 15, 6 |
| Unreal Tournament | AS-Frigate | pass | 0 | 0 | 0 | 0 | matches | 0, 0 |
| Unreal Tournament | DM-Deck16][ | pass | 0 | 0 | 0 | 0 | matches | 0, 0 |
| Unreal Tournament | AS-Overlord | pass | 0 | 0 | 0 | 0 | matches | 0, 0 |
| Unreal Tournament | DM-Codex | pass | 0 | 0 | 0 | 0 | matches | 0, 0 |
| Unreal Tournament | CTF-Face | pass | 0 | 0 | 0 | 0 | matches | 0, 0 |
| Deus Ex | 00_Training | pass | 0 | 0 | 0 | 1 | matches | 0, 0 |
| Deus Ex | 01_NYC_UNATCOIsland | pass | 0 | 0 | 0 | 1 | matches | 0, 0 |
| Deus Ex | 02_NYC_BatteryPark | pass | 0 | 0 | 0 | 1 | matches | 0, 0 |
| Deus Ex | 03_NYC_UNATCOHQ | pass | 0 | 0 | 0 | 1 | matches | 0, 0 |
| Deus Ex | 04_NYC_Hotel | pass | 0 | 0 | 0 | 1 | matches | 0, 0 |

## Botmatch guard

The pitch cap reaches every non-player pawn, the Unreal Tournament bots included. `DM-Deck16][?game=Botpack.DeathMatchPlus?NumBots=4?Difficulty=3`, `--exit-after=300`, `--exec=5:Fire`, trace `GameInfo.Killed,GameInfo.ScoreKill,Pawn.Died,Pawn.KilledBy`: 105 lines containing `GameInfo.Killed:` (the floor of the guard is 9). This run had ten bots. The reference run, on an earlier build, had four and 18 kills, so 105 against 18 compares two populations and is not evidence that the engine got deadlier: the stock `DeathMatchPlus` takes its bot count from the `InitialBots` setting in the game folder's ini and reads no `NumBots` option, so the `NumBots=4` in the command above sets nothing, and the folder's ini held ten when this run was made. Killers by `ScoreKill`: 36 with no killer, then ten bots between 3 and 14 kills each (`TFemale1Bot1` 14, `TMale1Bot0` 9, `TFemale2Bot1` 9, `TMale1Bot2` 8, ...). The player died at about 17.6 seconds of the smoke log's clock. `SUMMARY`: 0 script errors, 0 stub hits, 0 missing natives, 0 exceptions, 0 warnings, `fps_avg=1095.2`.

## What these runs do not cover

- Body pitch has no run-level observable, since no harness command prints a rotation. The unit tests close it.
- What the original keeps for a walking non-player pawn's body that aims up or down is open; `TickRotateTo` and its zeroing of a walking pawn's desired pitch are unchanged, and flying and swimming pawns, which the cap also reaches, are equally unverified.
- Because the cap holds a desired pitch above the rate away from the desired rotation, `EndedRotation` is no longer raised for such a pawn; no stock script of the three games handles that event.
- The `LongFall` of a monster, and the Predator's flight, which is stubbed again: `GetLastVisible`, `AICanHear`, `AICanSmell`, spider reach and the other natives left as stubs are untouched.
- The original game was not read. The iterator semantics and the `LongFall` threshold were designed, and the iterator's design was found insufficient.
