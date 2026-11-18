# Begin-play at load

These runs check one divergence from upstream's map load. The begin-play loops of `Engine::LoadMap` (PreBeginPlay, BeginPlay, PostBeginPlay, SetInitialState and InitBase) visit every actor of the level, including the actors that InitGame and the begin-play events spawn during the load, so such an actor ran its begin-play events once from its own spawn and again from the loops. Unreal Tournament's Actor.uc declares `bScriptInitialized` "to prevent re-initializing of actors spawned during level startup" and SetInitialState sets it, so the original does not run them twice. `SurrealEngine/StartupInit.h` holds the decision: an actor is skipped when its index is at or past the actor count taken before InitGame and its flag is set, and the flag is read only when the game's Actor class declares it. Unreal Gold and Deus Ex declare no such flag and keep upstream's two passes: Deus Ex needs the second SetInitialState (an inventory item that a carcass hides after its own spawn returns to its state through it), and whether the original Unreal Gold sends PostBeginPlay twice is not settled by its script.

## Recipe

Every run is a headless one, and every run uses the same flags: `--autostart --exit-after=<seconds> --frame-limit=0 --smoke-log=<log> --url=<url> --trace-script=<functions> "<game folder>"`, with the engine as the only client of a nested compositor with no input device (`WLR_BACKENDS=headless WLR_LIBINPUT_NO_DEVICES=1 cage -- <engine>`), 50 seconds each. Tools/smoke/README.md describes the flags. The engine log is the plaintext `SE-Log-LastRun.txt` of the run's configuration folder, which is copied beside the smoke log.

The pins are per run and never edit a game folder or the host's configuration. An Unreal Gold URL carries `?Class=UPak.UPakFemaleOne`. An Unreal Tournament run takes its bot count from a configuration folder the run owns, written by `python3 Tools/smoke/runpins.py prepare --game "<Unreal Tournament folder>" --kind ut --config-root <root> --bots <n>` (0 for every row but CTF-Tutorial, 4 for it) and read back from the engine log after the run by `runpins.py check`, which must print `CONFIG_DIR ok`, `BOTS <n> ok` and exit 0. The Deus Ex folder is the vanilla 1112fm one.

The trace filters are `F_BEGIN` = `Actor.PreBeginPlay,Actor.BeginPlay,Actor.PostBeginPlay,Actor.SetInitialState` and `F_DRAW` = `Object.FRand,GameInfo.Login,GameInfo.PostLogin,GameInfo.FindPlayerStart`. A trace line is `[<seconds>] <caller>: trace: <label> <Class> <state>: <Function>(<args>)`, where the caller is the script function named by the frame below the top one. The trace stops after 20000 calls with one `trace: stopped after 20000 calls` line, and a run that hits it inside the load is repeated with fewer functions and never judged.

Every comparison is over the load segment, the trace lines before the first `GameInfo.Login` line: the begin-play loops run there, while later spawns depend on frame timing and differ between two runs of one binary. Builds compared: the reference is the source before upstream's loop was widened to `Level->Actors.size()` (it takes a count before InitGame and runs each loop to it), the base is the tree without the guard, the node is the tree with it.

| Row | Game | URL | Filter |
| :--- | :--- | :--- | :--- |
| deck16 | Unreal Tournament | `DM-Deck16][` | `F_DRAW,F_BEGIN` |
| face | Unreal Tournament | `CTF-Face?game=Botpack.CTFGame` | `F_BEGIN,GameInfo.Login` |
| tut | Unreal Tournament, 4 bots | `CTF-Tutorial` | `F_BEGIN,GameInfo.Login` |
| vortex2, nyleve | Unreal Gold | `Vortex2`, `NyLeve` (with the class pin) | `F_BEGIN,GameInfo.Login` |
| upak | Unreal Gold | `DmAriza?game=UPak.UPakGame` (with the class pin) | `F_BEGIN,GameInfo.Login,RocketLauncher.PostBeginPlay,UPakGame.UPakGameSetup` |
| nyc | Deus Ex | `01_NYC_UNATCOIsland` | `F_BEGIN,GameInfo.Login,DeusExCarcass.AddInventory` |

## Helper scripts

Standard library only; run them as `python3 -I -B <script> ...`. `tracelib.py` is the parser the others share. The fixtures in `fixture/` are a hand-made reference engine log (`reference.log`), a second-pass one (`second-pass.log`, one more PreBeginPlay of a mutator and one more draw of the game info before login) and a smoke log (`smoke.log`).

- `frand.py <engine log> [<smoke log>]` counts the `FRand` lines before the first `Login` line and the first `PostLogin` line, prints one `CALLER <n> <label> <Class> via <caller>` line per source of a draw before login, and the first player spawn from the first `STATE` line of the smoke log. `CAPPED_BEFORE_POSTLOGIN 1` voids the counts. On the fixture, `reference.log` prints `FRAND_BEFORE_LOGIN 3`, `FRAND_BEFORE_POSTLOGIN 4`, `CALLER 2 gameinfo DeathMatchPlus via Actor.PreBeginPlay`, `CALLER 1 <unnamed Foo> Foo via DeathMatchPlus.InitGame` and `FIRST_SPAWN ('909.3', '1916.3', '-634.6')`; `second-pass.log` prints 4, 5 and `CALLER 3 gameinfo DeathMatchPlus via Actor.PreBeginPlay`.
- `tracecount.py count <engine log> [--whole]` prints `<label> <Class> <Function> <n>` per object and function, and `tracecount.py compare <reference log> <subject log> [--whole]` prints a `DIFF` line for a count that differs among the objects present in both logs, `ONLY_REFERENCE` and `ONLY_SUBJECT` for objects in one log only, and `COUNTS_EQUAL` or `COUNTS_DIFFER`. On the fixture, `compare reference.log second-pass.log` prints `DIFF Mutator0 Mutator PreBeginPlay reference 1 subject 2`, `DIFF gameinfo DeathMatchPlus FRand reference 2 subject 3` and `COUNTS_DIFFER objects_common 3 only_reference 0 only_subject 0`; compared with itself it prints `COUNTS_EQUAL objects_common 3 only_reference 0 only_subject 0`.
- `tracediff.py <reference log> <subject log> [--whole]` compares the trace lines after the bracketed seconds are stripped and prints `TRACE_IDENTICAL <n>` (exit 0) or `TRACE_DIFFERS` with the first differences (exit 1). On the fixture, `reference.log` against itself prints `TRACE_IDENTICAL 4` and against `second-pass.log` prints `TRACE_DIFFERS reference 4 subject 6`.

`--whole` compares the whole run and is reported beside the load segment, never judged.

## Expected values

Draws before login on `DM-Deck16][` with no bots (the bot-free configuration folder), judged with `frand.py`:

| Build | `FRAND_BEFORE_LOGIN` | `FRAND_BEFORE_POSTLOGIN` | Callers before login | `FIRST_SPAWN` |
| :--- | :--- | :--- | :--- | :--- |
| reference and node | 535 | 558 | `525 gameinfo DeathMatchPlus via Actor.PreBeginPlay`, `8 gameinfo UTIntro via Actor.PreBeginPlay`, `2 gameinfo DeathMatchPlus via Mover.PostBeginPlay` | `('909.3', '1916.3', '-634.6')` |
| base (no guard) | 537 | 560 | `526`, `9` and `2` of the same three | `('-161.6', '903.5', '-661.3')` |

The placed movers' own draws (the third caller) are never skipped by the guard. `tracecount.py compare` of the reference against the node prints `COUNTS_EQUAL` on all three Unreal Tournament maps (603, 490 and 283 objects in common on `DM-Deck16][`, `CTF-Face` and `CTF-Tutorial`), and against the base it prints `COUNTS_DIFFER` on `DM-Deck16][` with 22 `DIFF` lines, among them `DMMutator0 DMMutator PostBeginPlay reference 2 subject 4` and `gameinfo DeathMatchPlus FRand reference 527 subject 528`. With four bots, `CTF-Tutorial` prints 200 `Accessed None` lines of `TrainingCTF.SpawnRatedBot` on the reference and on the node.

On every Unreal Gold and Deus Ex row, `tracediff.py <base log> <node log>` prints `TRACE_IDENTICAL` over the load segment (2886 lines on `Vortex2`, 4121 on `NyLeve`, 2059 on `DmAriza` with the UPak game, 10906 on `01_NYC_UNATCOIsland`), and no log holds `Property offset out of bounds`, which an unguarded read of the flag would produce on the first actor. Read from the traces and not judged: no `BabyCow` object has a line in the `NyLeve` load segment; `DmAriza` with the UPak game spawns two launchers whose `PostBeginPlay` trace lines number 8 (`RocketLauncher0`) and 4 (`RocketLauncher1`) on both builds; `01_NYC_UNATCOIsland` has nine `AddInventory` lines from carcasses, and with `Inventory.BeginState,Inventory.EndState` added to the filter the second `SetInitialState` of a carcass item (`Candybar0`) ends its state `Pickup`, on both builds.

## When to retire the guard

The guard is a standing divergence from upstream's loops and every landing that touches those loops re-checks it: a conflict on the five loop lines of `Engine::LoadMap` is resolved by keeping upstream's text and re-applying the guard call, then rerunning the rows above. Delete `StartupInit.h`, its calls and `tests/level/StartupInitTests.cpp` when the loops stop visiting actors spawned during the load, or when the stock scripts show that an actor whose SetInitialState has run is meant to run its begin-play events again. To extend the guard to another game, its Actor class must declare the flag and its own runs (the rows above on that game's maps) must show the second pass is not wanted; Deus Ex's carcass items need the second SetInitialState, so it must not be extended without a replacement for that call.
