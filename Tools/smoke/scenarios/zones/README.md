# Zone runs

These runs drive stock maps of Unreal Gold 226b, Unreal Tournament 436 and Deus Ex 1112fm to look at two things the status document listed as buggy: semisolid brushes ("Usually you'll fall through them as if they weren't there") and portals, the warp zones and teleporters ("they somewhat work, but might end up pushing players/projectiles in unexpected directions"). The first part sets a player down over the top face of a semisolid brush, at points chosen from the map's own data, and reads whether it stands or falls. The second part drops a pawn into a warp zone, walks a pawn through a warp doorway, shoots a projectile into a warp zone and walks a pawn onto a teleporter pad, and reads where each ends up and how it moves afterwards. The player is put where a scenario needs it with the harness's `place` command.

In the portal tables, `Unchanged engine` is the engine built from the source of the commit that adds this file, whose engine source is that of d8687f8e (the commit before it adds only a test and changes no engine source). `With the placement change` is a build in which `SetLocation` also raises the actor's teleport flag and updates its zone, and the physics step clears the flag at its start for every physics mode; that change is not in the tree (see "Why the placement change is not in the tree"). The semisolid table has one column, `Binary`: the CTF-Nucleus points gave the same outcome on the unchanged engine and on the build with the placement change, and the SkyBase points were run on the unchanged engine only.

## Recipe

Every run is an unattended smoke run, started with the same flags and no `--user-dir` (the engine then writes no user files and nothing into the game folder):

```
SurrealEngine --autostart --exit-after=<seconds> --smoke-log=<log> --url=<map> \
    --exec='<items>' --trace-script=<entries> "<game folder>"
```

Tools/smoke/README.md describes the flags, `hold`, `call` and `place`. What these runs add to them:

- A run is one engine on a quiet host, and events are timed by the engine log's stamps, not the `STATE` lines'. An event is counted from the stamp of the `PLACE` line on. The runs are not deterministic: bots and monsters change what they shoot and where they stand from run to run.
- A position probe: `call LineOfSightTo class:<PlayerClass>` logs `call: class:<PlayerClass> resolved to <ObjectName> at (x, y, z)`. The player classes are `UPakFemaleOne` (Unreal Gold) and `TMale2` (Unreal Tournament). A bot of the player's class answers the same probe, so the player is the object whose first probe after the `PLACE` line agrees with the placed location in x and y.
- An Unreal Tournament match starts with the console `Fire` command, sent 5 s after login. Unreal Gold starts playing without it (its player is ready from about 4.4 s). `place` is used instead of `Ghost` and `Walk`, which cannot put a player where a brush or a portal is: a ghost has no collision and touches nothing, dies outside the level's geometry in Unreal Tournament, and does nothing in Deus Ex.
- The trace: `--trace-script` writes one line per call to each traced class in the instance's class chain, so a call that goes through `Super` prints two lines with the same text after `trace: `. A warp zone's `ActorEntered` and `ActorLeaving` both do, so an entry of an actor into a warp zone is a pair of consecutive lines and the counts below are in entries; the `UnWarp` and `Warp` natives print one line each, and a plain zone's `ActorEntered`, a teleporter's `Touch` and `Accept` and a pawn's `Died` count as the log prints them. The log stops after 20000 traced calls.
- The physics step is the real frame time, 0.4 to 3 ms on the machine of these runs, so a distance that depends on it is a range, and a threshold is in units or seconds of the engine, never in ticks. Each run's own `FPS` lines are in its smoke log.
- A weapon: in Unreal Tournament `Summon Botpack.PulseGun` puts a pulse gun 72 units ahead, walking over it picks it up and `SwitchWeapon 5` selects it; `hold axis aLookUp` with a negative value pitches the view down (2048 turns it 90 degrees a second, and the stock clamp holds the pitch at -90 degrees) and the console `Fire` shoots.
- `<game folder>` is a vanilla install of the game; a Deus Ex install with a mod folder is not used.

## Semisolid brushes

The engine finds what a walking or falling player is standing on with its box trace, which reads the leaf hulls of the level model: a node that has a hull is a convex solid of the planes the hull lists, and a node without one adds nothing to the box trace. A ray, which carries shots and sight checks, instead walks the polygons of the nodes and skips only those marked not solid. About a third of the polygons of every game carry the semisolid flag (34.6, 33.4 and 31.7 percent in Unreal Gold, Unreal Tournament and Deus Ex), and a semisolid brush's polygons contribute planes to the hulls around them as solid ones do, so the box trace is not blind to a semisolid brush; but a polygon with no hull of its own stops a box only where a neighbouring hull happens to carry its plane. The runs ask what a player does where the map holds a polygon with a hull, a polygon without one and no polygon at all.

### How the points were chosen

The points come from the map data, not from where a player can be walked. A brush with a flat, up-facing top of platform size and both hulled and hull-less top polygons was read from the level model (the top polygons, their nodes and hull offsets, and for each the number of hulls in its column that carry an up-facing plane at the top height). Three points on the top: the centroid of the hulled top polygon whose column holds the fewest hulls with the top plane, the centroid of the hull-less one with the fewest, and the grid point inside the outline of the tops that lies in no top polygon and whose column holds no hull with the top plane. Each point is set down 60 units above the top height and the player is read for 8 s. A point is used only where no pawn the map places stands within 800 units, because a bot or a monster that damages the player voids the run; every run was traced for damage to the player, and none shows any but the fall's own.

The player stands when its centre settles at the top height plus its collision height (39; the engine leaves a player 1 unit above that) within a second and stays within 2 units. It falls when its centre passes below the top height and settles lower, or it dies of the fall.

In Unreal Tournament the brush is the slab in the middle of CTF-Nucleus (`Brush185`, a ring of top polygons at z 384 around an open centre). It is the engine's faithfulness case, not a candidate for a fix: played by hand in the original game by the fork's owner on 2026-10-22, the ring holds a player and the centre lets it fall through, and in this engine's level data the centre holds no polygon and no hull. Of the eight Unreal Gold and seven Deus Ex brushes that fit the description, only SkyBase's platform (`Brush1521`) has all three points clear of every placed pawn; every Deus Ex brush has a placed pawn within 800 units of one of its points, so none was used.

### Results

| Game | Map and brush | Point | Polygon under it | Binary |
| :--- | :--- | :--- | :--- | :--- |
| Unreal Tournament | CTF-Nucleus, slab (top z 384) | (28.0, 3516.0) | top polygon with a hull | stands: the centre settles at z 424 within 0.3 s and stays |
| Unreal Tournament | CTF-Nucleus, slab | (29.3, 3505.2) | top polygon with no hull | stands: z 424 within 0.3 s (the box lands on the hulls of neighbouring polygons) |
| Unreal Tournament | CTF-Nucleus, slab | (-16.0, 3504.0) | none | falls: the centre passes z 384 on the way down and settles at z 64 after 1.1 s |
| Unreal Gold | SkyBase, platform (top z 2208) | (-4393.6, -873.6) | top polygon with a hull | stands: z 2248 within 0.3 s |
| Unreal Gold | SkyBase, platform | (-4288.0, -618.7) | top polygon with no hull | sinks: z 2232 within 0.5 s, 16 units below where a player stands over the hulled polygon (the box passes the top face and rests on the hull of another brush whose top is at z 2192) |
| Unreal Gold | SkyBase, platform | (-4288.0, -768.0) | none | not read: the column is outside the level (zone 0) from z 2288 down, and a player placed there dies of falling out of the world at once |
| Deus Ex | none | | | no brush qualifies |

The CTF-Nucleus outcomes (stand, stand, fall) are the same on the unchanged engine and on the build with the placement change. The fork's owner's hand play of the original game, which covered the ring and the centre, found the same (it is not a run of these three points). The two polygons are held by hulls: a port of the box trace over the level's real hulls lands the box at the top on all 39 top polygons of the slab and on nothing at the centre, and the runs agree. A player placed on a neighbouring floor about 100 units from the hulled point and walked toward it for 2 s crosses the slab's eastern perimeter without sinking and walks off the far side.

SkyBase shows the other thing the hull-only trace does: the top face of a polygon with no hull does not stop the box, which goes on until another brush's hull holds it. The same port puts the box at that height on five of the platform's six hull-less top polygons. A fall all the way through a semisolid top face was not found on any map tried, so the status line says so and narrows the symptom to this case. The box trace is unchanged by this work.

Runs: Unreal Tournament, `--url='CTF-Nucleus?game=Botpack.CTFGame'`, trace `Pawn.TakeDamage,Pawn.Died`, `--exit-after=40`, one run per point, then a probe of `class:TMale2` every 0.25 s from 7.75 s to 16.25 s:

```
5:Fire;8:place 28.0 3516.0 444
```

(and `8:place 29.3 3505.2 444`, `8:place -16 3504 444`). The walk-on run is `5:Fire;8:place 40.0 3612.0 423 262.9;8.5:hold axis aForward 300 2`. Unreal Gold, `--url=SkyBase`, the same trace, a probe of `class:UPakFemaleOne`, with no `Fire`:

```
8:place -4393.6 -873.6 2268
```

(and `8:place -4288.0 -618.7 2268`, `8:place -4288.0 -768.0 2268`).

## Portals

A warp zone moves an actor that enters it to its paired zone through the stock script (`WarpZoneInfo.ActorEntered`, which turns the actor's location, velocity and rotation by the two zones' coordinate systems); a teleporter does the same from its `Touch` and the destination's `Accept`. Both guard on the actor's `bJustTeleported` flag, which the stock engine raises when a script moves an actor, so that an actor entering the zone it was moved into is not moved again. Three facts decide what the runs below can show.

- The warp zones of DM-Fractal cannot be walked into. The zones on the `out` side are closed on every side but the top and the ones on the `in` side on every side but the underside (the vertical pair is open on one z face), so a pawn reaches them only by dropping from the shaft above, and the walked crossing is made in Unreal Gold's DmRadikus, whose warp zone `water-left` is open on its +x face and whose floor goes on through the doorway.
- In the tree, `SetLocation` raises no flag and updates no zone. The walking and projectile ticks set an actor's velocity to its displacement over the tick unless the flag is raised, so an actor that a warp zone's or a teleporter's script moves inside such a tick is given a velocity of about the jump over the tick, which the next tick cuts to its top speed along the jump: a pawn is thrown along the jump, and a projectile sent along it. The stock guard reads a flag nothing raises, so the entry into the far zone warps the actor back. A falling actor does not recompute its velocity unless it hits a slope, so a drop through a pit is not pushed.
- The flag is cleared only at the start of a pawn's walking, ladder, swimming and flying ticks and of every falling, rolling and projectile tick. The build with the placement change raises it in `SetLocation` (for every class of actor, once the level has begun play), updates the zone through the actor's own `UpdateActorZone` after the touch loops, and clears the flag at the start of every physics step, so an actor in a mode with no clear of its own does not keep it raised for good.

The physics step is the frame time, so the pushes below are ranges over the step. A crossing is never made by placing the player inside a zone: the pawn walks, falls or is shot in.

### Drops through DM-Fractal's pits

The player is set down in the shaft above a pit with no input held and falls into the pit's warp zone. A falling actor is not pushed, so both binaries must give the same single warp.

| Pit | Place | Jump | Unchanged engine | With the placement change |
| :--- | :--- | :--- | :--- | :--- |
| Right (`WarpZoneInfo2` to `WarpZoneInfo3`) | (1186.8, -454.5, 629.9) | (-400.0, +8.0, 0.0), to the column (786.8, -446.5); the exit is below the far zone (it spans z 568 to 632 there) and the player settles at z 359 on the platform | two runs, each 1 entry of the player on `WarpZoneInfo2` 0.36 s after the placement, 1 `UnWarp` and 1 `Warp`, none on `WarpZoneInfo3`, no second entry; the first probe after the warp is at (786.8, -446.5, 487.6) and (786.8, -446.5, 486.9), 0.0 units from the column | the same count; the first probe is at (786.8, -446.5, 486.7), 0.0 units from the column, and the player rests at (786.8, -446.5, 360.0); `ActorLeaving` for the pit comes inside the warp's own move |
| Left (`WarpZoneInfo4` to `WarpZoneInfo5`) | (-577.6, -441.0, 630.55) | (+400.0, -8.0, 0.0), to the column (-177.6, -449.0), below the far zone (z 572 to 636) | two runs, each 1 entry 0.35 s after the placement, 1 `UnWarp` and 1 `Warp`, none on `WarpZoneInfo5`, no second entry; the first probe after the warp is 5.6 units east of the column | the same count; every probe from the warp on is at (-177.6, -449.0), and the player rests at z 360.0 |

Bots stand near both exits (a weapon lies on the floor under the right one) and shoot the player as it lands. In every unchanged-engine run the later probes of both drops are moved (the traced re-runs show `RocketDeath` damage to the player within a second of the warp, and in the right pit's first re-run the player lands on a bot), so they are not read; the right pit's drop in the shot run below, where no bot reached the player, rests at (786.8, -446.5, 360.0) on the unchanged engine too. The right pit's run with the change rested at z 360.0 for two probes before a rocket moved the player.

### Walking through a warp doorway

DmRadikus, Unreal Gold: the warp zone `water-left` (`WarpZoneInfo5`, spanning x -1440 to -1360, y 1728 to 1856, z -64 to 112) is open on its +x face and paired with `balcony` (`WarpZoneInfo6`, spanning x 784 to 864, y -1088 to -960, z 0 to 176, open on its -x face). The stock script moves a walker through the first by (+2144.0, -2816.0, +64.0), azimuth -52.7 degrees, and leaves its heading unchanged. The player is placed at (-1160, 1792, -24) facing west, held at full forward for 0.3 s, released and placed 4 units outside the face at (-1356, 1792, -24); a placement keeps the velocity (about 375 units a second here), so the player enters the zone with no input held. `--url=DmRadikus`, trace `WarpZoneInfo.ActorEntered,Pawn.Died`, `--exit-after=14`, a probe of `class:UPakFemaleOne` every 0.1 s from 7.3 s to 12.5 s:

```
7:place -1160 1792 -24 180;7.05:hold axis aForward 6400 0.3;7.35:release;7.35:place -1356 1792 -24 180
```

| | Unchanged engine | With the placement change |
| :--- | :--- | :--- |
| Entries of the player | 5000 on `WarpZoneInfo5` and 5000 on `WarpZoneInfo6` before the 20000-call cap ends the log 3.9 s after the first, which is 10.6 ms after the placement | 1 on `WarpZoneInfo5`, none after it |
| Where the player is | alternating between the exit site (x 783.7 to 784.0, y -1024.1 to -1023.7, z 40.0) and the return site (x -1360.0 to -1359.7, y 1791.5 to 1791.9, z -24.0): a jump of (+2144, -2816) at azimuth -52.7 degrees out and 127.3 back | walking west (azimuth 180 degrees, y -1024.0, z 40.0): 12.5 units from the exit point 0.04 s after the entry, and 47.9 units from it after 5 s, still creeping |

The code predicts the unchanged engine's ping-pong (1650 to 12465 entries in 5 s over frame times of 3 to 0.4 ms; the run's frame rate gives 12465). The pawn with the change crosses once and keeps its heading. It comes to rest farther from the exit point than the speed the stock acceleration gives at the face predicts (12 to 17 units): the pawn crosses the last 4 units in 10.6 ms, about 375 units a second, not the 100 to 130 predicted, so it coasts about 47 units against a ground friction that takes 8 units a second off for each unit walked.

### Shooting into a pit

DM-Fractal, Unreal Tournament: the player picks up a pulse gun, pitches its view to -90 degrees, is placed at (1186.8, -454.5, 640) in the shaft above the right pit and fires straight down. The plasma sphere (1450 units a second) leaves the gun 12 units off the pit's column, enters the pit 0.06 s later and is moved by (-400.0, +8.0, 0.0) into the big room. Trace `WarpZoneInfo.ActorEntered,WarpZoneInfo.ActorLeaving,WarpZoneInfo.Warp,WarpZoneInfo.UnWarp,ZoneInfo.ActorEntered,Pawn.Died,Weapon.ProjectileFire,PlayerPawn.Fire,Projectile.Explode`, `--exit-after=60`, a probe of `class:TMale2` every 0.25 s from 6 s to 16 s and of `class:PlasmaSphere` every 0.02 s from 11 s to 12.5 s:

```
5:Fire;6:Summon Botpack.PulseGun;6.5:hold axis aForward 300 1.0;8.5:SwitchWeapon 5;9:hold axis aLookUp -2048 1.5;11:place 1186.8 -454.5 640;11.02:Fire
```

| | Unchanged engine | With the placement change |
| :--- | :--- | :--- |
| Shot after the warp | flies flat along the jump: between its first two probes (0.02 s apart) it travels 39.6 units at azimuth 178.8 degrees and elevation -1.3 degrees, and explodes on the rock 1035 units west at (-237.0, -414.4, 540.0) | goes on straight down (elevation -90.0 degrees, every probe at (798.7, -435.2)) and explodes on the floor of the big room, z 321 |

The unchanged engine's elevation is the step of the tick that carried the shot into the zone over the jump: the code predicts -0.1 to -0.9 degrees over the frame times of these runs, and a -1.3 degree elevation is a 6 ms tick.

### Walking onto a teleporter pad

DM-HyperBlast, Unreal Tournament: the pad `Teleporter0` (tag `32sucks`) at (-477.3, -1902.8) sends the player to `Teleporter1` at (478.2, -1902.0), a jump of (+955.5, +0.8, 0.0). The player is placed in the passage north of the pad, held at full forward for 0.3 s, released and placed 48 units north of the pad's centre, facing south; a placement keeps the velocity, so the player reaches the pad within 0.04 s with no input held. D is the vector from the arrival point to the player's probe 2.0 s after the pad's `Accept`. `--url=DM-HyperBlast`, trace `Teleporter.Touch,Teleporter.Accept,Pawn.Died`, `--exit-after=60`, a probe of `class:TMale2` every 0.1 s from 7.3 s to 12.5 s:

```
5:Fire;7:place -477.3 -1700 -56 270;7.05:hold axis aForward 6400 0.3;7.35:release;7.35:place -477.3 -1854.8 -56 270
```

| | Unchanged engine | With the placement change |
| :--- | :--- | :--- |
| `Touch` and `Accept` of the player | 1 each, 0.041 s after the placement; none after in 5 s | the same |
| D | (+24.9, -1.3), length 24.9, azimuth -3 degrees (a second run: (+25.0, -1.3)): thrown along the jump, as the code predicts | (0.0, +13.0), length 13.0 (a second run: (0.0, +12.9)): no push along the jump |

With the change the player rests 13 units from the arrival point on the side it came from, where the code predicts 11 to 12 units on the side it was heading to. Nothing traced in the pad's events, the player's landing, wall hits and zone changes, or the damage it takes shows why.

### Why the placement change is not in the tree

The build with the change removes the push in every measurement above, and kept the drops and the CTF-Nucleus points as they were. The regression sample (below) did not clear it:

- Falls out of the world rose on the build with the change on Harobed, in both runs (0 to 1 and 0 to 5 lines; a fly's death prints two lines, a swarm's one), and in one of two runs on NyLeve (2 to 4) and Chizra (0 to 2). The traces show the following. In both Harobed runs `HorseFlySwarm1` entered `LevelInfo0` (the zone actor of a zone that has none of its own, the outside zone among them) on a line whose source field is `<Surreal Engine>`, not a script, that is, at the end of its own move, and died of the fall 0.6 ms later (22.548 s and 24.585 s). In the second Harobed run `HorseFly22` (33.291 s) and `HorseFly17` (34.478 s) entered `LevelInfo0` on lines whose source field is `HorseFly.meander`, a state of the stock `HorseFly` script that moves the fly with `SetLocation` (`SetLocation(Owner.Location)` when it cannot see its owner, the swarm), and died in the same frame. On NyLeve `BiterFish17` entered `LevelInfo0` by its own move (source `<Surreal Engine>`, state `Flopping`) and died 0.8 ms later (31.027 s), in one run of two. On Chizra `HorseFly28` died at 24.254 s and no traced line names it before that; the trace ended at its 20000-call cap at 27.123 s. Without the change no traced line names `HorseFlySwarm1` in either Harobed run. The change updates the zone of an actor moved by a script at once, inside `SetLocation`, and the stock scripts read as if the engine did that: the warp zone's guard tests the teleport flag before its own `SetLocation`, the biter-fish school reads `Region.Zone` right after its `SetLocation`, and the horse fly declares `ZoneChange` singular around a `SetLocation`. What makes the swarm reach the outside zone only with the change is not established, so the rise in falls is recorded as unattributed.
- The zone-event counts of NyLeve (all three), Chizra (`FootZoneChange(`) and Harobed (`ActorEntered(` and `HeadZoneChange(`) stayed more than the larger of 20 lines and 25 percent of the unchanged engine's count away from it in a re-run too. The unchanged engine's own two runs differ from each other by more than that on the same maps (Chizra `ActorEntered(` 781 and 9354, Harobed `HeadZoneChange(` 94 and 7881, NyLeve `HeadZoneChange(` 113 and 642), because the creatures' behaviour changes from run to run; those counts alone could not separate a change from the variation, but the falls could.

## Regression sample

Twenty stock maps, 60 seconds each, on both binaries, as the committed sweeps ran them (`--url=<map>`, no other option, except that the Unreal Tournament maps whose prefix the committed sweep mapped to a game ran as `AS-Frigate?game=Botpack.Assault`, `AS-Overlord?game=Botpack.Assault`, `CTF-Face?game=Botpack.CTFGame` and `CTF-High?game=Botpack.CTFGame`), judged with the sweep runner's own `parse_log` and `classify`. `matches` means the verdict and the counts of script errors, exceptions, missing natives and stub hits equal the committed row of the same map. Trace `ZoneInfo.ActorEntered,Pawn.FootZoneChange,Pawn.HeadZoneChange,Pawn.Died`.

| Game | Map | Verdict | Script errors | Exceptions | Missing natives | Stub hits | Match |
| :--- | :--- | :--- | ---: | ---: | ---: | ---: | :--- |
| Unreal Gold | Vortex2 | pass | 0 | 0 | 0 | 0 | matches on both binaries |
| Unreal Gold | Dig | pass | 0 | 0 | 0 | 0 | matches on both binaries |
| Unreal Gold | NyLeve | pass | 0 | 0 | 0 | 0 | matches on both binaries |
| Unreal Gold | Chizra | pass | 0 | 0 | 0 | 0 | matches on both binaries |
| Unreal Gold | Harobed | pass | 0 | 0 | 0 | 0 | matches on both binaries |
| Unreal Gold | Bluff | pass | 0 | 0 | 0 | 0 | matches on both binaries |
| Unreal Gold | SkyTown | pass | 0 | 0 | 0 | 0 | matches on both binaries |
| Unreal Tournament | AS-Frigate | pass | 0 | 0 | 0 | 0 | matches on both binaries |
| Unreal Tournament | DM-Deck16][ | pass | 0 | 0 | 0 | 0 | matches on both binaries |
| Unreal Tournament | AS-Overlord | pass | 0 | 0 | 0 | 0 | matches on both binaries |
| Unreal Tournament | DM-Codex | pass | 0 | 0 | 0 | 0 | matches on both binaries |
| Unreal Tournament | CTF-Face | pass | 0 | 0 | 0 | 0 | matches on both binaries |
| Unreal Tournament | CTF-High | pass | 0 | 0 | 0 | 0 | matches on both binaries |
| Unreal Tournament | DM-KGalleon | pass | 0 | 0 | 0 | 0 | matches on both binaries |
| Deus Ex | 00_Training | pass | 0 | 0 | 0 | 1 | matches on both binaries |
| Deus Ex | 01_NYC_UNATCOIsland | pass | 0 | 0 | 0 | 1 | matches on both binaries |
| Deus Ex | 02_NYC_BatteryPark | pass | 0 | 0 | 0 | 1 | matches on both binaries |
| Deus Ex | 03_NYC_UNATCOHQ | pass | 0 | 0 | 0 | 1 | matches on both binaries |
| Deus Ex | 04_NYC_Hotel | pass | 0 | 0 | 0 | 1 | matches on both binaries |
| Deus Ex | 09_NYC_Ship | pass | 0 | 0 | 0 | 1 | matches on both binaries |

The zone-event table counts trace lines (a pawn's `Died` and a warp zone's `ActorEntered` print two) for the unchanged engine and the build with the placement change, in that order. Four maps ran twice because a counter or the falls moved; the log of a run is capped at 20000 traced calls, so the counts of a capped run are lower bounds.

| Map | Run | `ActorEntered(` (unchanged engine, with change) | `FootZoneChange(` | `HeadZoneChange(` | Falls (`Died` of damage type fell) |
| :--- | :--- | ---: | ---: | ---: | ---: |
| Vortex2 | 1 | 0, 0 | 0, 0 | 0, 0 | 0, 0 |
| Dig | 1 | 11, 37 | 16, 32 | 4, 4 | 0, 0 |
| Dig | 2 | 18, 38 | 8, 9 | 4, 4 | 0, 0 |
| NyLeve | 1 | 110, 189 | 18, 55 | 113, 701 | 2, 4 |
| NyLeve | 2 | 80, 399 | 14, 259 | 642, 1215 | 2, 2 |
| Chizra | 1 (log capped) | 781, 4138 | 1005, 3824 | 10528, 12034 | 0, 2 |
| Chizra | 2 (log capped) | 9354, 8434 | 411, 7237 | 10233, 4327 | 0, 0 |
| Harobed | 1 | 202, 383 | 16, 18 | 94, 65 | 0, 1 |
| Harobed | 2 | 49, 335 | 13, 12 | 7881, 2548 | 0, 5 |
| Bluff | 1 | 2, 8 | 2, 3 | 1, 1 | 2, 2 |
| SkyTown | 1 | 261, 310 | 186, 70 | 258, 193 | 0, 0 |
| AS-Frigate | 1 | 17, 10 | 0, 0 | 0, 0 | 0, 0 |
| DM-Deck16][ | 1 | 0, 0 | 0, 0 | 0, 0 | 0, 0 |
| AS-Overlord | 1 | 81, 91 | 0, 1 | 0, 1 | 0, 0 |
| DM-Codex | 1 | 0, 0 | 0, 0 | 0, 0 | 0, 0 |
| CTF-Face | 1 | 0, 0 | 0, 0 | 0, 0 | 0, 0 |
| CTF-High | 1 | 0, 0 | 0, 0 | 0, 0 | 0, 0 |
| DM-KGalleon | 1 | 0, 0 | 0, 0 | 0, 0 | 0, 0 |
| 00_Training | 1 | 0, 0 | 0, 0 | 0, 0 | 0, 0 |
| 01_NYC_UNATCOIsland | 1 | 0, 0 | 0, 0 | 0, 0 | 0, 0 |
| 02_NYC_BatteryPark | 1 | 8, 10 | 1, 1 | 0, 0 | 0, 0 |
| 03_NYC_UNATCOHQ | 1 | 0, 0 | 0, 0 | 0, 0 | 0, 0 |
| 04_NYC_Hotel | 1 | 0, 0 | 0, 0 | 0, 0 | 0, 0 |
| 09_NYC_Ship | 1 | 0, 1 | 0, 1 | 0, 1 | 0, 0 |
