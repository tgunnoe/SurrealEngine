# Mover runs

These runs drive stock maps of Unreal Gold 226b, Unreal Tournament 436 and Deus Ex 1112fm to show how the engine treats a mover (a moving brush), and trace the stock scripts that decide what follows. A mover that runs into an actor must reach the stock script, `Mover.EncroachingOn`, with that actor, so that the script crushes, stops or returns as its `MoverEncroachType` says. A mover also blocks what hits it: a projectile fired at a mover calls the stock `HitWall` with the mover as `Wall`, and a walking player who runs into a mover slides along it instead of stopping dead. The engine only reports the right hits and honours the script's answer; the answer itself is the script's.

Each section says what its `Previous binary` and `Fixed binary` columns are. In the three per-game encroachment sections they are an engine built before the gather fix and one built from the fix. In `Projectile against a mover` and `Walking into a mover` they are the build at the merge of the encroachment change and the build of the blocking change. `Encroachment re-runs` and `Regression sample` use the build of the blocking change only.

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
- Weapons: a shot is started with the `Fire` console command; `hold button bFire` only sets the button and starts nothing. The Unreal Gold player starts without a weapon in Vortex2: `Summon UnrealShare.DispersionPistol` places one 72 units ahead, walking over it picks it up and `SwitchWeapon 1` selects it. In Unreal Tournament `Summon Botpack.PulseGun` and `SwitchWeapon 5` do the same, and `hold axis aLookUp` with a negative value pitches the view down. An Unreal Tournament match has bots (`TMale1Bot0` and the like) and starts the player at a start that is the same in every run of a map.
- A projectile's stop is read from the trace: `HitWall(?, <mover>)` (a vector prints as `?`) is the stock `HitWall` called with the mover as `Wall`, and `Touch(<mover>)` followed by `ProcessTouch` and `TakeDamage` is the projectile touching the mover and exploding through `Projectile.Touch` instead. `call LineOfSightTo name:<Mover>` logs a mover's origin.
- The stock `Actor.HitWall` has no body, so a player's own `HitWall` leaves no trace line; a walking player's slide is read from the position probes.

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

## Projectile against a mover

A projectile fired at a closed or stationary mover must be stopped by it, because the mover's own block flags decide: the projectile calls the stock `Projectile.HitWall` with the mover as `Wall`. Both columns come from the same choreography; the previous binary is the build at the merge of the encroachment change and the fixed binary the build of the blocking change. The Unreal Gold and Unreal Tournament runs fire twice.

| Game | Map | Mover | Weapon | Previous binary | Fixed binary |
| :--- | :--- | :--- | :--- | :--- | :--- |
| Unreal Gold 226b | Vortex2 | Mover44, the ceiling fixture, once it has landed | dispersion pistol, summoned | both shots touch the fixture: `Touch(Mover44)`, `ProcessTouch(Mover44, ?)`, `Mover44` `TakeDamage(15, ..., 'exploded')` and `Explode`; no `HitWall` names it | both shots end in `Mover44.Bump`, `HitWall(?, Mover44)` and `Explode`; nothing touches the fixture and it takes no damage (it is not damage-triggered) |
| Unreal Tournament 436 | DM-Deck16][ | Mover0, the right lift, closed | pulse gun, summoned | both shots touch the lift: `Touch(Mover0)`, `ProcessTouch(Mover0, ?)`, `Mover0` `TakeDamage(20, ..., 'Pulsed')` and `Explode`; no `HitWall` names it | both shots end in `Mover0.Bump`, `HitWall(?, Mover0)` and `Explode`; nothing touches the lift |
| Deus Ex 1112fm | 00_Training | none | none | not reached: the map's weapons are a knife and a crowbar, and `Summon` spawned nothing in the run | not reached |

Unreal Gold, `--url=Vortex2`, trace `Projectile.HitWall,Projectile.Explode,Projectile.Touch,Projectile.ProcessTouch,Mover.Bump,Mover.TakeDamage,Mover.Trigger,Weapon.ProjectileFire,PlayerPawn.Fire`, then a probe of `class:UPakFemaleOne` and `name:Mover44` every 0.25 s from 5 s to 14 s, `--exit-after=17`:

```
5:Summon UnrealShare.DispersionPistol;5.3:hold axis aForward 300 1.5;7:SwitchWeapon 1;7.2:hold axis aForward 37 1.7;7.2:hold axis aStrafe -298 1.7;9.2:CauseEvent litefall3;11:Fire;11.8:Fire
```

The player walks over the summoned pistol and selects it, then walks to a spot about 90 units south of the fixture's landing place (the player ends at about x -2087, y 180). `CauseEvent litefall3` drops the fixture, the same tag as the crush run above, and it stays down for the rest of the run. The two shots fly along the start view (128 degrees) into the south face of the landed fixture.

Unreal Tournament, `--url=DM-Deck16][`, trace `Projectile.HitWall,Projectile.Explode,Projectile.Touch,Projectile.ProcessTouch,Mover.Bump,Mover.TakeDamage,Weapon.ProjectileFire,PlayerPawn.Fire,Inventory.Touch`, then a probe of `class:TMale2` and `name:Mover0` every 0.25 s from 6 s to 15 s, `--exit-after=18`:

```
5:Fire;6:Summon Botpack.PulseGun;6.5:hold axis aForward 300 1.0;8.5:SwitchWeapon 5;9:Ghost;9.1:hold axis aLookUp -512 4;9.2:hold axis aStrafe 300 0.54;13.2:Fire;13.8:Fire
```

The player picks up the pulse gun at the start, goes ghost, pitches the view down and strafes over the right lift's shaft (to about x 1266, y 1836, as in the encroachment run above, but no one lands on the lift, so it stays closed at the bottom) and fires straight down at the lift's top face from about 600 units above it.

## Walking into a mover

A walking player who runs into a mover must slide along its face instead of stopping dead. The measure is the straight-line distance between the player's probe once it has been against the mover (the first `Bump` naming the player comes just before) and its probe after the hold ended, with the same choreography on both binaries. The previous binary is the build at the merge of the encroachment change and the fixed binary the build of the blocking change.

| Game | Map | Mover | Choreography | Previous binary | Fixed binary |
| :--- | :--- | :--- | :--- | :--- | :--- |
| Unreal Gold 226b | Vortex2 | Mover44, the ceiling fixture, once it has landed | walk to the west of its landing place, drop the fixture, walk into its west face at 45 degrees for 2.5 s | 0.0 units: the first `Bump` is at 11.23 s and the player stays at (-2295.0, 323.1) from 11.25 s on (probes 11.5 s to 13.0 s) | 155.6 units, from (-2291.6, 301.2) at 11.5 s to (-2226.9, 159.7) at 13.0 s: the player slides along the face (the first `Bump` is at 11.22 s) and past the fixture's corner |
| Unreal Tournament 436 | DM-Deck16][ | none | not reached: the only movers of this map are lifts 16 units thick, which the player steps over | not reached | not reached |
| Deus Ex 1112fm | 00_Training | DeusExMover32 and DeusExMover49, the locked doors in the start room's west wall | walk backwards and sideways into the door at 45 degrees for 3 s | 0.0 units: the first `Bump` is at 5.65 s and the player stays at (-1167.4, 844.0) (probes 5.75 s to 9.0 s) | 25.9 units, from (-1168.0, 849.1) at 5.75 s to (-1168.1, 875.0) at 9.0 s, along the door up to its frame |

The Deus Ex slide is short because the door sits in a recessed frame whose edge closes the face about 30 units north of where the player meets the door. In Unreal Tournament the player reaches no mover with a face to walk into from the start of DM-Deck16][ (the start of DM-HealPod][ is on another floor than its pod, and a ghost flight through rock kills the player there), and the doors of AS-Overlord are rotating movers, which this change does not cover. A walking player was blocked by a mover and slid along it in Unreal Gold and Deus Ex, and a projectile was stopped by a mover in Unreal Gold and Unreal Tournament.

Unreal Gold, `--url=Vortex2`, trace `Mover.Bump,Mover.Trigger,Actor.HitWall`, then a probe of `class:UPakFemaleOne` and `name:Mover44` every 0.25 s from 5 s to 16 s, `--exit-after=17`:

```
5:hold axis aForward 289 2.7;5:hold axis aStrafe -80 2.7;9:CauseEvent litefall3;10.4:hold axis aForward -298 2.5;10.4:hold axis aStrafe -35 2.5
```

The player walks north-west to about (-2322, 351), 35 units west of where the fixture lands, the fixture is dropped at 9 s and lands at about 10.2 s, and the player walks east-south-east at 45 degrees into its west face from 10.4 s to 12.9 s. The first `Bump` is at 11.2 s, so the reference probe is the one at 11.5 s and the end probe the one at 13.0 s.

Deus Ex, `--url=00_Training`, trace `Mover.Bump,Mover.Trigger,Mover.EncroachingOn`, then a probe of `class:JCDentonMale`, `name:DeusExMover49` and `name:DeusExMover32` every 0.25 s from 4.5 s to 9 s, `--exit-after=11`:

```
5:hold axis aForward -212 3;5:hold axis aStrafe 212 3
```

The player starts 40 units east of the doors facing east and walks backwards and to its left (north), so it meets the door at 45 degrees; the reference probe is the one at 5.75 s and the end probe the one at 9.0 s.

## Encroachment re-runs

The five encroachment runs above, repeated on the build of the blocking change with the same choreography, trace and exit time and the probes the recipe describes, to show that the encroach gates answer as before. Each run is compared with the count of the section it comes from.

| Run | Compared count | Band | Found |
| :--- | :--- | :--- | :--- |
| Unreal Gold, Mover44 crush | `EncroachingOn(UPakFemaleOne0)` followed by `KilledBy(UPakFemaleOne0)` | exactly 1 | 1 (and `FemaleOneCarcass0` 3 times, not compared) |
| Unreal Gold, Mover12 lift | `Bump` and `Attach` for the player; `EncroachingOn` | 1 and 1; 0 | 1 and 1; 0 |
| Unreal Tournament, Mover0 under the player | `EncroachingOn(TMale20)` and `MakeGroupReturn` | 3 and 3 | 3 and 3 |
| Unreal Tournament, Mover0 over a pawn | `EncroachingOn` naming the pawn (`TMale10`) and the passenger (`TMale20`) | 4 and 0 | 4 and 0 |
| Deus Ex, the `UN_maindoor` pair | `EncroachingOn(JCDentonMale0)` and `EncroachedBy(DeusExMover16)` | equal to within 2 lines and within 20 percent of 1217 (974 to 1460) | 1233 and 1233 |

## Regression sample

Fifteen stock maps, 60 seconds each, on the build of the blocking change, as the committed sweeps ran them (`--url=<map>`, no other option, except that the three Unreal Tournament maps whose prefix the committed sweep mapped to a game ran as `AS-Frigate?game=Botpack.Assault`, `AS-Overlord?game=Botpack.Assault` and `CTF-Face?game=Botpack.CTFGame`), judged with the sweep runner's own `parse_log` and `classify`. `matches` means the verdict and the counts of script errors, exceptions, missing natives and stub hits equal the committed row of the same map. The script warnings are not compared; the column gives the committed count and the count of this run. Their kinds are the ones the committed analysis already holds (`HorseFly.meander`, `SinglePlayer.Killed`, and in Dig one `SinglePlayer.ScoreKill`), and the counts of the flies change from run to run.

| Game | Map | Verdict | Script errors | Exceptions | Missing natives | Stub hits | Match | Script warnings (committed, run) |
| :--- | :--- | :--- | ---: | ---: | ---: | ---: | :--- | :--- |
| Unreal Gold | Vortex2 | pass | 0 | 0 | 0 | 0 | matches | 0, 0 |
| Unreal Gold | Dig | pass | 0 | 0 | 0 | 0 | matches | 15, 51 |
| Unreal Gold | NyLeve | pass | 0 | 0 | 0 | 0 | matches | 0, 0 |
| Unreal Gold | Chizra | pass | 0 | 0 | 0 | 0 | matches | 17, 20 |
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
