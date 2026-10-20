# Mover encroachment runs

A moving brush (a mover) that runs into an actor must reach the stock script, `Mover.EncroachingOn`, with that actor, so that the script crushes, stops or returns as its `MoverEncroachType` says. These runs drive stock maps of Unreal Gold 226b, Unreal Tournament 436 and Deus Ex 1112fm so that a mover meets the player (or another actor), trace the script, and compare an engine built before the gather fix (the previous binary) with one built from the fix (the fixed binary). The engine only calls the script with the right candidates and honours its answer; the answer itself is the script's.

## Recipe

Every run is an unattended smoke run, started with the same flags and no `--user-dir` (the engine then writes no user files and nothing into the game folder):

```
SurrealEngine --autostart --exit-after=<seconds> --smoke-log=<log> --url=<map> \
    --exec='<items>' --trace-script=<entries> "<game folder>"
```

Tools/smoke/README.md describes the flags, `hold` and `call`. What these runs add to them:

- The engine log (`SE-Log-LastRun.txt`, written when the engine exits) holds one `trace:` line per traced call. A run is read by counting the `EncroachingOn(<player object name>)` lines and the lines that follow them (`MakeGroupReturn`, `MakeGroupStop`, `KilledBy`, `EncroachedBy`).
- A position probe: `call LineOfSightTo class:<PlayerClass>` logs `call: class:<PlayerClass> resolved to <ObjectName> at (x, y, z)`, the actor's location at that moment (`name:<ObjectName>` does the same for a named actor such as a mover). The player classes are `UPakFemaleOne` (Unreal Gold), `TMale2` (Unreal Tournament) and `JCDentonMale` (Deus Ex). Adding one probe every 0.25 to 0.5 s to the `--exec` list gives a trajectory of the run; the start location and view direction of these maps were the same in every run, so a hold of a given length reaches about the same place each time, but the runs are not deterministic and a position varies by a few units.
- Placing the player: in Unreal Gold and Unreal Tournament the console commands `Ghost` (fly through walls, no collision) and `Walk` (solid again, falling to the floor) work in a standalone game, and `CauseEvent <tag>` triggers every actor with that tag. `hold axis aForward`, `aStrafe` and `aUp` move the player relative to its view yaw (Unreal Gold's start yaw is about 128 degrees, Unreal Tournament's on DM-Deck16][ is -90 degrees; a positive `aStrafe` is toward (-sin yaw, cos yaw)); a ghost flies at about 516 units a second in Unreal Gold and 660 in Unreal Tournament. `hold axis aTurn` turns the view by about 17.5 degrees per `aTurn` of 512 held for one second in Deus Ex and 22.5 in Unreal Tournament. In Unreal Tournament the match starts with the console `Fire` command. In Deus Ex `Ghost` has no effect, and its doors open by frob: `call FrobTarget.Frob self none` frobs what the view points at.

## Unreal Gold 226b

| Map | Mover | Choreography | Previous binary | Fixed binary |
| :--- | :--- | :--- | :--- | :--- |
| Vortex2 | Mover44 (tag `litefall3`, crush) | U1: stand under a ceiling fixture that falls | 0 `EncroachingOn(UPakFemaleOne0)`, no `KilledBy`; the player is alive at the end | 1 `EncroachingOn(UPakFemaleOne0)` followed by `KilledBy(UPakFemaleOne0)`; the player is `Dying` from 14.9 s; 1 more `EncroachingOn` for the carcass |
| Vortex2 | Mover12 (tag `liftthis`, stand-open lift) | U2: ride the lift up and down | `Bump` and `Attach` for the player; the player rises 208 units and comes back; 0 `EncroachingOn` | the same: the passenger does not reach the script, because it rests above the lift's top face |

U1 (trace `Mover.Bump,Mover.EncroachingOn,Mover.MakeGroupReturn,Mover.MakeGroupStop,Pawn.EncroachedBy,Pawn.TakeDamage,Pawn.KilledBy,Pawn.gibbedBy`), then a probe every 0.5 s from 5 s to 22 s, `--exit-after=26`:

```
5:hold axis aForward 300 3.2;10:hold axis aForward -239 2.6;10:hold axis aStrafe -181 2.6;14:CauseEvent litefall3
```

The player walks to the room under the fixture, stands in the box the fixture lands in (x -2287 to -2111, y 269 to 419) and the fixture is triggered at 14 s.

U2 (trace `Mover.Bump,Mover.Attach,Mover.EncroachingOn,Mover.MakeGroupReturn,Mover.MakeGroupStop,Pawn.EncroachedBy,Pawn.TakeDamage,Pawn.KilledBy,Pawn.gibbedBy`), then a probe every 0.25 s from 5 s to 24 s, `--exit-after=28`:

```
5:Ghost;5.5:hold axis aForward 256 2.07;5.5:hold axis aStrafe 156 2.07;8:Walk
```

The ghost flies to the middle of the lift's platform (x -3264 to -3008, y 224 to 352) and walks onto it.

## Unreal Tournament 436

| Map | Mover | Choreography | Previous binary | Fixed binary |
| :--- | :--- | :--- | :--- | :--- |
| DM-Deck16][ | Mover0 (tag `Right`, stand-open lift) | T1: ride the lift up, fly down into the shaft, stand under it as it comes back | 0 `EncroachingOn`; the lift passes through the player (`Bump` only) | 3 `EncroachingOn(TMale20)`, each followed by `MakeGroupReturn()` (the lift returns upward each time) |
| DM-Deck16][ | Mover0 | T2: the player rides the lift down as a passenger onto a pawn that floats in the shaft | 0 `EncroachingOn` | 4 `EncroachingOn(TMale10)` (the pawn), each followed by `MakeGroupReturn()`; 0 for the passenger |

T1 (trace `Mover.Bump,Mover.Attach,Mover.EncroachingOn,Mover.MakeGroupReturn,Mover.MakeGroupStop,Pawn.EncroachedBy,Pawn.TakeDamage,Pawn.KilledBy,Pawn.gibbedBy`), then a probe every 0.25 s from 6 s to 26 s, `--exit-after=30`:

```
5:Fire;6:Ghost;6.5:hold axis aStrafe 300 0.54;7.2:hold axis aUp -300 0.88;8.3:Walk;10.3:Ghost;10.6:hold axis aUp -300 0.8;11.8:Walk
```

T2 (the same trace), probes of `TMale2`, `TMale1` and `name:Mover0` every 0.25 s from 8 s to 26 s, `--exit-after=30`:

```
5:Fire;6:Ghost;6.5:hold axis aStrafe 300 0.54;7.2:hold axis aUp -300 0.88;8.3:Walk;10.6:Ghost;10.9:hold axis aUp -300 0.8;11.9:Summon Botpack.TMale1;12.2:hold axis aUp 300 0.8;13.2:Walk
```

The player lands on the lift and rises with it. In T2 the player then flies down, summons a pawn (which stays where it is spawned, in the lift's path), flies back up and lands on the lift again before it comes down. The lift stops on the pawn with the player aboard, so the stop path moves the passenger back with the lift. Over the 38 probes taken while the player rides, its height above the lift's origin stays between 41.1 and 41.9 units (the two probes of a pair are a frame apart). An engine built from the fix with the passenger restore removed shows the mover meeting its own passenger instead: the passenger sinks into it a step at a time (the height ranges from 40.4 to 45.2 units) and the log holds 210 `EncroachingOn` entries naming the player beside 213 for the pawn (423 `MakeGroupReturn`), where the fixed binary has 4 for the pawn and none for the player.

## Deus Ex 1112fm

| Map | Mover | Choreography | Previous binary | Fixed binary |
| :--- | :--- | :--- | :--- | :--- |
| 00_Training | DeusExMover16 and DeusExMover17 (tag `UN_maindoor`, sliding door pair, ignore type) | D1: open the door, stand in the doorway, close it | 0 `EncroachingOn(JCDentonMale0)`, 0 `EncroachedBy` | 1217 `EncroachingOn(JCDentonMale0)` lines, each with an `EncroachedBy(DeusExMover16)`; the ignore type answers `false` in the script, so nothing else is traced |

D1 (trace `Mover.Trigger,Mover.EncroachingOn,Mover.MakeGroupReturn,Mover.MakeGroupStop,Pawn.EncroachedBy,Pawn.TakeDamage,Pawn.KilledBy,Pawn.gibbedBy,DeusExMover.Frob,DeusExMover.Trigger`), then a probe every 0.5 s from 5 s to 26 s, `--exit-after=30`:

```
5:hold axis aForward 300 6;11.2:call FrobTarget.Frob self none;13.5:hold axis aForward 300 0.6;15:hold axis aTurn -2633 1;17.2:call FrobTarget.Frob self none
```

The player walks east to the closed door, frobs it open, steps into the doorway, turns to face the edge of one leaf, and frobs again, which closes both leaves on the player. Every door of this map is of the ignore type, so the script lets the door go through the player.

## Regression sample

Fifteen stock maps, 60 seconds each, on the fixed binary, as the committed sweeps ran them (`--url=<map>`, no other option), judged with the sweep runner's own `parse_log` and `classify`. `matches` means the verdict and the counts of script errors, exceptions, missing natives and stub hits equal the committed row of the same map. The script warnings are not compared; the column gives the committed count and the count of this run. Their kinds are the ones the committed analysis already holds (`HorseFly.meander`, `SinglePlayer.Killed`, `Slith.TakeDamage`), and the counts of the flies change from run to run on the previous binary too (Harobed 15 against 6 committed, Dig 17 against 15).

| Game | Map | Verdict | Script errors | Exceptions | Missing natives | Stub hits | Match | Script warnings (committed, run) |
| :--- | :--- | :--- | ---: | ---: | ---: | ---: | :--- | :--- |
| Unreal Gold | Vortex2 | pass | 0 | 0 | 0 | 0 | matches | 0, 0 |
| Unreal Gold | Dig | pass | 0 | 0 | 0 | 0 | matches | 15, 29 |
| Unreal Gold | NyLeve | pass | 0 | 0 | 0 | 0 | matches | 0, 0 |
| Unreal Gold | Chizra | pass | 0 | 0 | 0 | 0 | matches | 17, 18 |
| Unreal Gold | Harobed | pass | 0 | 0 | 0 | 0 | matches | 6, 12 |
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
