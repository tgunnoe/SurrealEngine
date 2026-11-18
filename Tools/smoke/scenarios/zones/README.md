# Zone runs

These runs drive stock maps of Unreal Gold 226b, Unreal Tournament 436 and Deus Ex 1112fm to look at two things the status document listed as buggy: semisolid brushes ("Usually you'll fall through them as if they weren't there") and portals, the warp zones and teleporters ("they somewhat work, but might end up pushing players/projectiles in unexpected directions"). The first part sets a player down over the top face of a semisolid brush, at points chosen from the map's own data, and reads whether it stands or falls. The second part drops a pawn into a warp zone, walks a pawn through a warp doorway, shoots a projectile into a warp zone and walks a pawn onto a teleporter pad, and reads where each ends up and how it moves afterwards. The player is put where a scenario needs it with the harness's `place` command.

In the portal tables, `Previous binary` is the engine built from the commit before the change that makes `SetLocation` a teleport (the fifth upstream landing's merge), and `Fixed binary` is the engine with that change: `SetLocation` raises the actor's teleport flag and updates its zone once the level has begun play, the physics step clears the flag at its start for every physics mode, and `SetLocation` on a deleted actor moves nothing and returns false. The semisolid table has one column, `Binary`: the CTF-Nucleus points gave the same outcome on the previous and the fixed binary, and the SkyBase points were run on the unchanged engine only.

## Recipe

Every run is an unattended smoke run, started with the same flags and no `--user-dir` (the engine then writes no user files and nothing into the game folder):

```
SurrealEngine --autostart --exit-after=<seconds> --smoke-log=<log> --url=<map> \
    --exec='<items>' --trace-script=<entries> "<game folder>"
```

Tools/smoke/README.md describes the flags, `hold`, `call` and `place`, and the three per-run pins every run here carries. What these runs add to them:

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

The CTF-Nucleus outcomes (stand, stand, fall) are the same on the previous and the fixed binary. The fork's owner's hand play of the original game, which covered the ring and the centre, found the same (it is not a run of these three points). The two polygons are held by hulls: a port of the box trace over the level's real hulls lands the box at the top on all 39 top polygons of the slab and on nothing at the centre, and the runs agree. A player placed on a neighbouring floor about 100 units from the hulled point and walked toward it for 2 s crosses the slab's eastern perimeter without sinking and walks off the far side.

SkyBase shows the other thing the hull-only trace does: the top face of a polygon with no hull does not stop the box, which goes on until another brush's hull holds it. The same port puts the box at that height on five of the platform's six hull-less top polygons. A fall all the way through a semisolid top face was not found on any map tried, so the status line says so and narrows the symptom to this case. The box trace is unchanged by this work.

Runs: Unreal Tournament, `--url='CTF-Nucleus?game=Botpack.CTFGame'`, trace `Pawn.TakeDamage,Pawn.Died`, `--exit-after=40`, one run per point, then a probe of `class:TMale2` every 0.25 s from 7.75 s to 16.25 s:

```
5:Fire;8:place 28.0 3516.0 444
```

(and `8:place 29.3 3505.2 444`, `8:place -16 3504 444`). The walk-on run is `5:Fire;8:place 40.0 3612.0 423 262.9;8.5:hold axis aForward 300 2`. Unreal Gold, `--url=SkyBase?Class=UPak.UPakFemaleOne`, the same trace, a probe of `class:UPakFemaleOne`, with no `Fire`:

```
8:place -4393.6 -873.6 2268
```

(and `8:place -4288.0 -618.7 2268`, `8:place -4288.0 -768.0 2268`).

## Portals

A warp zone moves an actor that enters it to its paired zone through the stock script (`WarpZoneInfo.ActorEntered`, which turns the actor's location, velocity and rotation by the two zones' coordinate systems); a teleporter does the same from its `Touch` and the destination's `Accept`. The warp zone's script guards on the actor's `bJustTeleported` flag, so that an actor entering the zone it was moved into is not moved again. The teleporter's script does not read the flag: its destination turns off its own `Touch` while it moves the actor, and it sets the arriving velocity, which the physics step keeps only while the flag is raised. Three facts decide what the runs below can show.

- The warp zones of DM-Fractal cannot be walked into. The zones on the `out` side are closed on every side but the top and the ones on the `in` side on every side but the underside (the vertical pair is open on one z face), so a pawn reaches them only by dropping from the shaft above, and the walked crossing is made in Unreal Gold's DmRadikus, whose warp zone `water-left` is open on its +x face and whose floor goes on through the doorway.
- Before the change, `SetLocation` raised no flag and updated no zone. The walking and projectile ticks set an actor's velocity to its displacement over the tick unless the flag is raised, so an actor that a warp zone's or a teleporter's script moves inside such a tick was given a velocity of about the jump over the tick, which the next tick cut to its top speed along the jump: a pawn was thrown along the jump, and a projectile sent along it. The stock guard read a flag nothing raised, so the entry into the far zone warped the actor back. A falling actor does not recompute its velocity unless it hits a slope, so a drop through a pit was not pushed.
- With the change, `SetLocation` raises the flag (for every class of actor, once the level has begun play) and then updates the zone through the actor's own `UpdateActorZone` after the touch loops, so the zone events fire with the flag already raised. The physics step clears the flag at its start for every mode, so an actor in a mode with no clear of its own does not keep it raised for good. A move that a script makes between two physics steps, from an actor's `Tick` or its state code, computes its next velocity as before but now updates the zone at once with the flag raised; the runs contain such moves (the held weapon that `Weapon.RenderOverlays` moves in the doorway run, and the flies' `meander` state code on Harobed).

A crossing is never made by placing the player inside a zone: the pawn walks, falls or is shot in, since the placement itself now raises the flag and the stock warp zone script skips an actor whose flag is raised. A teleporter script does not read the flag, so the flag does not stop a placement that touches a teleporter, or enters a zone that forwards entries to one, from being teleported; no run here places a player on a pad. Every run was made in a headless compositor with no input device, with the per-run pins described under "Pinned runs", on a binary built from the commit named in the column head. The recipes below place the player at the same points as the harness pins and step the probes by 0.25 s (the pit and slab recipes) where the pins step by 0.5 s: `zn-w1` is the right pit, `zn-w2` the left pit, and `zn-ctf1` to `zn-ctf3` the three Nucleus points, each on the same map and at the same placed point with the same trace, so the two recipes read one outcome and are not one recipe.

### Drops through DM-Fractal's pits

The player is set down in the shaft above a pit with no input held and falls into the pit's warp zone. A falling actor is not pushed, so both binaries must give the same single warp. The right pit sends the player by (-400.0, +8.0, 0.0) to the column (786.8, -446.5) and the left pit by (+400.0, -8.0, 0.0) to the column (-177.6, -449.0); trace `WarpZoneInfo.ActorEntered,WarpZoneInfo.ActorLeaving,WarpZoneInfo.Warp,WarpZoneInfo.UnWarp,ZoneInfo.ActorEntered,Pawn.Died`, a probe of `class:TMale2` every 0.25 s from 7.75 s to 16.25 s, the placements at 8 s after the `5:Fire` that starts the match.

| Pit | Place | Previous binary | Fixed binary |
| :--- | :--- | :--- | :--- |
| Right (`WarpZoneInfo2` to `WarpZoneInfo3`) | (1186.8, -454.5, 629.9) | 1 entry of the player 0.363 s after the placement, 1 `UnWarp` and 1 `Warp`, none on `WarpZoneInfo3`, no second entry in 5 s; the first probe after the warp is 0.0 units from the column | the same count and the same distance |
| Left (`WarpZoneInfo4` to `WarpZoneInfo5`) | (-577.6, -441.0, 630.55) | 1 entry 0.353 s after the placement, the same events, 0.0 units from the column | the same |

### Walking through a warp doorway

DmRadikus, Unreal Gold: the warp zone `water-left` (`WarpZoneInfo5`, spanning x -1440 to -1360, y 1728 to 1856, z -64 to 112) is open on its +x face and paired with `balcony` (`WarpZoneInfo6`, spanning x 784 to 864, y -1088 to -960, z 0 to 176, open on its -x face). The stock script moves a walker through the first by (+2144.0, -2816.0, +64.0) and leaves its heading unchanged. The player is placed at (-1160, 1792, -24) facing west, held at full forward for 0.3 s, released and placed 4 units outside the face at (-1356, 1792, -24); a placement keeps the velocity, so the player enters the zone with no input held. `--url=DmRadikus?Class=UPak.UPakFemaleOne`, trace `WarpZoneInfo.ActorEntered,WarpZoneInfo.UnWarp,WarpZoneInfo.Warp,Pawn.Died`, `--exit-after=21`, a probe of `class:UPakFemaleOne` every 0.1 s from 7.3 s to 12.5 s:

```
7:place -1160 1792 -24 180;7.05:hold axis aForward 6400 0.3;7.35:release;7.35:place -1356 1792 -24 180
```

| | Previous binary | Fixed binary |
| :--- | :--- | :--- |
| Entries of the player | 2500 on `WarpZoneInfo5` and 2500 on `WarpZoneInfo6` (4999 after the first) before the 20000-call trace cap ends the log 1.6 s after the first, which is 10.4 ms after the placement | 1 on `WarpZoneInfo5`, none after it |
| Entry speed from the stamps of the placement and the entry, and the rest the stock friction predicts (speed over 8) | 385.7 units a second, 48.2 | 381.4 units a second, 47.7 |
| Where the player is | alternating between the exit site and the return site | walking west (azimuth 180.0 degrees), 47.3 units from the exit point (784.0, -1024.0) at the probe 5 s after the entry; the judge's band was 27.0 to 68.4 |
| Entries of other actors on the two zones | none | 6, all of the player's dispersion pistol (`Weapon.RenderOverlays` moves the held weapon with `SetLocation`), none of them warped |

The previous binary ping-pongs thousands of times a second; the fixed binary crosses once and keeps its heading. Whether the original game fires the zone events for a held weapon is not known; the weapon's entries are counted and gate nothing.

### Shooting into a pit

DM-Fractal, Unreal Tournament: the player picks up a pulse gun, pitches its view to -90 degrees, is placed at (1186.8, -454.5, 640) in the shaft above the right pit and fires straight down. The plasma sphere (1450 units a second) enters the pit and is moved by (-400.0, +8.0, 0.0) into the big room. Trace `WarpZoneInfo.ActorEntered,WarpZoneInfo.ActorLeaving,WarpZoneInfo.Warp,WarpZoneInfo.UnWarp,ZoneInfo.ActorEntered,Pawn.Died,Weapon.ProjectileFire,PlayerPawn.Fire,Projectile.Explode`, `--exit-after=25`, a probe of `class:TMale2` every 0.25 s from 6 s to 16 s and of `class:PlasmaSphere` every 0.02 s from 11 s to 12.5 s:

```
5:Fire;6:Summon Botpack.PulseGun;6.5:hold axis aForward 300 1.0;8.5:SwitchWeapon 5;9:hold axis aLookUp -2048 1.5;11:place 1186.8 -454.5 640;11.02:Fire
```

| | Previous binary | Fixed binary |
| :--- | :--- | :--- |
| Shot after the warp | flies flat along the jump: elevation -0.4 degrees, azimuth 178.9 degrees over its first probes after the warp, the sphere 1000.2 units from the exit column at its farthest | goes on straight down: elevation -90.0 degrees, never more than 16.3 units from the exit column |

### Walking onto a teleporter pad

DM-HyperBlast, Unreal Tournament: the pad `Teleporter0` (tag `32sucks`) at (-477.3, -1902.8) sends the player to `Teleporter1` at (478.2, -1902.0), a jump of (+955.5, +0.8, 0.0). Both pads have yaw 16496 and the class defaults (`bChangesYaw` true, nothing else set), so the stock `Accept` turns an arriving pawn through half a turn and re-points its velocity onto the new heading: a pawn that arrives southbound leaves northbound. The player is placed in the passage north of the pad, held at full forward for 0.3 s, released and placed 48 units north of the pad's centre, facing south; a placement keeps the velocity, so the player reaches the pad within 0.04 s with no input held. D is the vector from the arrival point to the player's probe 2.0 s after the pad's `Accept`. `--url=DM-HyperBlast`, trace `Teleporter.Touch,Teleporter.Accept,Pawn.Died`, `--exit-after=21`, a probe of `class:TMale2` every 0.1 s from 7.3 s to 12.5 s:

```
5:Fire;7:place -477.3 -1700 -56 270;7.05:hold axis aForward 6400 0.3;7.35:release;7.35:place -477.3 -1854.8 -56 270
```

| | Previous binary | Fixed binary |
| :--- | :--- | :--- |
| `Touch` and `Accept` of the player | 1 each, 0.0413 s after the placement; none after in 5 s | 1 each, 0.0408 s after the placement; none after |
| D | (+25.0, -1.5), length 25.0, azimuth -3.4 degrees: thrown along the jump | (0.0, +12.8), length 12.8, azimuth 90.2 degrees: coasts north from the arrival point, where the coasting prediction from the stamps is 14.1 |

### The CTF-Nucleus pins

The three Nucleus points (28.0, 3516.0), (29.3, 3505.2) and (-16.0, 3504.0), placed 60 units over the slab at z 444 at 8 s: the first two stand (the centre settles at z 424 within 2 units from 1.5 s on) and the third falls (below z 384 and settling at or below z 100), on both binaries.

### A trailer along

W1's recipe with one change: before the placement the player summons `Botpack.UT_ShieldBelt` and walks over it, which spawns a `UT_ShieldBeltEffect` owned by the player. The effect's default physics is `PHYS_Trailer`, whose tick calls `SetLocation(Owner.Location)` on every step, so with the change every trailer tick raises the flag and runs the zone update. Trace as for the pit drops plus `UT_ShieldBeltEffect.ZoneChange,UT_ShieldBeltEffect.Destroyed` (the effect's own `ZoneChange` has no body in the stock scripts, so the call is never traced).

| | Previous binary | Fixed binary |
| :--- | :--- | :--- |
| Summon line | at 6.599 s | at 6.599 s |
| Zone events naming the effect | 0 | 2, entering `LevelInfo0` and then `ZoneInfo0`; 0 entries on the warp zones, the effect going from the level's zone straight to the destination zone at its owner's warp; no `Destroyed` line |
| The player's pit pin | held | held |

Two runs of each binary gave the same counts. The pair cannot reach everything. Trailer mode has no zone-0 test, so a trailer cannot fall out of the world, and the flag's readers are not in the trailer tick. The stock zone handlers a trailer can now reach are `ZoneInfo.ActorEntered`, which moves only `PHYS_None` effects; `WarpZoneInfo`, guarded by the flag; `CloudZone`, which destroys any non-pawn that enters; `TarZone`, `NitrogenZone` and `VacuumZone`, which act on pawns only; and `TeleporterZone`, which calls its teleporter's `Touch`. The `Effects` subclasses that override `ZoneChange` are three in Unreal (`Bubble`, `Bubble1`, `SparkBit`), five in Unreal Tournament (the same three, `UT_Spark`, `UT_Sparks`) and one in Deus Ex (`FlyGenerator`, which destroys nothing); none is known to be a trailer.

## Falls out of the world

The first version of this change was kept out of the tree because falls out of the world rose on Harobed with it, unattributed. This section is the attribution. The runs are three of each binary on Harobed, NyLeve and Chizra (60 s, Unreal Gold with the class pin), traced for every call of `FellOutOfWorld`, `Died`, `SetPhysics`, `SetLocation` and `Destroyed` of the flock classes and for the zone entries into the zone object that zone 0 maps to (`LevelInfo`), and Harobed's six swarms are probed every 2 s. Chizra's `BiterFish.SetPhysics` alone filled the 20000-call trace cap on a first run (it capped at 52.6 s and again at 48.8 s in a re-run), so its list names the other flock classes' `SetPhysics` and not `BiterFish`'s: a biter fish that falls on Chizra would read its physics mode as unread. Both binaries' Chizra runs use that list.

A fall death is an actor's `FellOutOfWorld` call. Deaths are pooled per map and class and compared by an exact binomial test: for B deaths on the previous binary and N on the fixed one over equal run counts, p is the probability of N or more of the B+N deaths landing on the fixed binary at chance one half. An ambient class (the stock `FlockPawn` and `FlockMasterPawn` families) is judged elevated when p is at most 0.05, and every other class when it has a death on the fixed binary and none on the previous one. A first comparison at three runs a side asks for three more runs a side when an ambient class has p at most 0.25; none did. For a class that is elevated each death that a script's move zoned would need a root, an earlier zone-0 entry of another actor; none was elevated. The rule has an accepted residual: for one ambient class with equal rates on both binaries it calls a false elevation 0.2 to 2.6 percent of the time per class per map and asks for the extension 7 to 16 percent of the time; it detects a class that dies once a run on the fixed binary and never on the previous one 65 percent of the time, a doubling from 1 to 2 a run 27 percent, and a tripling from 1 to 3 a run 67 percent. An effect smaller than that lands as "not shown to differ".

| Map | Class | Deaths, previous binary (3 runs) | Deaths, fixed binary (3 runs) | p | Verdict |
| :--- | :--- | ---: | ---: | ---: | :--- |
| Harobed | `HorseFly` | 3 | 0 | 1.0 | not shown to differ |
| Harobed | `HorseFlySwarm` | 0 | 0 | 1.0 | no death on either binary |
| NyLeve | `Bird1` | 3 | 3 | 0.656 | not shown to differ |
| NyLeve | `BiterFish` | 0 | 1 | 0.5 | not shown to differ |
| Chizra | all | 0 | 0 | 1.0 | no death on either binary |

All three maps end `ATTRIBUTED` at the first comparison. The deaths read: on Harobed the previous binary's three are flies in `meander` in flying physics that entered the zone-0 object by their own move (at 21.65 s, 29.22 s and 5.93 s); the fixed binary has none. Five of the swarms called `SeePlayer` within 0.2 s of the start in every run of both binaries and `DeadBodySwarm0` between 22 and 30 s, so the swarms were active in every run, and none died on either binary: the swarm death of the first version did not recur. On NyLeve `Bird14` is placed in zone 0 and dies at about 0.018 s in every run of both binaries with no entry line; the fixed binary's one `BiterFish` death (`BiterFish21`, `Flopping`, falling physics, 29.9 s) entered the zone-0 object by its own move. Because the swarm shows no death on either binary, the forced-activation pair was not run. What the first version's rise in falls was is not established by these runs: with 3 runs a binary it was not reproduced.

The fork's rule for the event, by physics mode, set against the stock scripts. The stock overrides of `FellOutOfWorld` are eight, read from every package of the three games that holds the name: `Actor` (`SetPhysics(PHYS_None); Destroy();`) and `Pawn` in all three games (Unreal Gold's body is `Health = -1; SetPhysics(PHYS_None); Died(None, 'fell', Location);`; the Unreal Tournament and Deus Ex bodies add lines: Unreal Tournament's returns first when `Role < ROLE_Authority`, clears `Weapon` and uses the damage type `'Fell'`, Deus Ex's sets `Weapon = None` and uses `'Fell'`), and in Unreal Tournament also `BotPack.CTFFlag` twice, its base override (`SendHome()`) and an empty override in its state `Held`, where the flag has `PHYS_None` and no world collision. No stock script calls the event. The census reads the stock scripts and the fork; it does not say where the original raises the event, which stays open. A class's default physics is not in the script dumps, so a mode a class holds by default and never sets is not counted. The deaths columns read the last traced `SetPhysics` of the dying actor: previous binary and fixed binary, three runs of each on Harobed, NyLeve and Chizra.

| Mode | The fork at zone 0 | Unreal Gold `SetPhysics` | Unreal Tournament | Deus Ex | Deaths, previous | Deaths, fixed |
| :--- | :--- | :--- | :--- | :--- | ---: | ---: |
| `PHYS_None` | no test | 83 in 55 | 89 in 58 | 24 in 14 | 0 | 0 |
| `PHYS_Walking` | pawns only, in the pawn prelude | 21 in 14 | 16 in 11 | 2 in 1 | 0 | 0 |
| `PHYS_Falling` | any actor, at the start of the tick | 104 in 62 | 105 in 65 | 40 in 18 | 0 | 1 |
| `PHYS_Swimming` | pawns only, in the pawn prelude | 24 in 14 | 24 in 14 | 7 in 2 | 0 | 0 |
| `PHYS_Flying` | pawns only, in the pawn prelude | 23 in 12 | 25 in 12 | 9 in 4 | 6 | 3 |
| `PHYS_Rotating` | nothing (the mode does nothing) | 8 in 8 | 10 in 7 | 1 in 1 | 0 | 0 |
| `PHYS_Projectile` | destroys the actor, no event | 10 in 7 | 8 in 5 | 1 in 1 | 0 | 0 |
| `PHYS_Rolling` | any actor, at the start of the tick | 0 in 0 | 0 in 0 | 4 in 4 | 0 | 0 |
| `PHYS_Interpolating` | no test | 3 in 3 | 3 in 3 | 4 in 4 | 0 | 0 |
| `PHYS_MovingBrush` | no test | 1 in 1 | 1 in 1 | 1 in 1 | 0 | 0 |
| `PHYS_Spider` | no test | 4 in 2 | 4 in 2 | 0 in 0 | 0 | 0 |
| `PHYS_Trailer` | no test | 0 in 0 | 0 in 0 | 0 in 0 | 0 | 0 |

(The ladder is walking with a ladder flag.) No death was read in a mode the table does not raise.

The destroyed-actor pin: in every flock log of both binaries no zone or touch line with a script's source field names an actor after its `Destroyed` line (0 lines on the fixed binary's nine logs and 0 on the previous binary's nine, and 0 lines with the engine's own source field in either set).

Counters, never gated: `ZoneInfo.ActorEntered` lines on Harobed were 27, 21 and 173 on the previous binary and 23, 523 and 322 on the fixed one, and `LevelInfo` entries 4, 4, 3 and 2, 5, 4; entries into `LevelInfo` on NyLeve 3, 5, 6 against 4, 6, 7, and on Chizra 63, 394, 10 against 136, 109, 152. The lines of those entries whose source field is not the engine's own were 0 on every previous run and on every fixed run but Chizra's, where each has 1. The counts vary run to run by more than the difference between the binaries.

Nested events: the stock `HorseFlySwarm.ZoneChange`, `HorseFly.ZoneChange` and `BiterFishSchool.ZoneChange` are `singular` around a `SetLocation`, and the VM does not enforce the modifier, so with the zone update inside `SetLocation` the events they would have blocked now nest. Enforcing it belongs to the statement semantics of the VM and is not part of this change.

## Pinned runs

The five Unreal Tournament runs below are pins, read with `Tools/smoke/pinjudge.py`, whose table is this one (a test makes the two equal).

Every Unreal Gold run carries `?Class=UPak.UPakFemaleOne` on its URL, which the engine reads before the folder's `User.ini`. Every Unreal Tournament run uses a config root whose `SE-UnrealTournament.ini` has `MinPlayers=0` and `InitialBots=0` (`InitialBots` is read from the ini only, so a pit drop is not shot at by bots), written by `runpins.py prepare --kind ut --bots 0`. Every run starts with `--frame-limit=0`. All five place the player after the `5:Fire` that starts the match.

Run one with `python3 -I -B Tools/smoke/pinjudge.py cmd <pin>` and read it with `python3 -I -B Tools/smoke/pinjudge.py judge <pin> <smoke log> <engine log>`. All five held on the engine of this change.

| Pin | Game | Url | Exec | Trace | Exit after (s) | Judge |
| :--- | :--- | :--- | :--- | :--- | ---: | :--- |
| `zn-w1` | Unreal Tournament 436 | `DM-Fractal` | `5:Fire;7:call LineOfSightTo class:TMale2;7.5:call LineOfSightTo class:TMale2;8:place 1186.8 -454.5 629.9;8:call LineOfSightTo class:TMale2;8.5:call LineOfSightTo class:TMale2;9:call LineOfSightTo class:TMale2;9.5:call LineOfSightTo class:TMale2;10:call LineOfSightTo class:TMale2;10.5:call LineOfSightTo class:TMale2;11:call LineOfSightTo class:TMale2;11.5:call LineOfSightTo class:TMale2;12:call LineOfSightTo class:TMale2;12.5:call LineOfSightTo class:TMale2;13:call LineOfSightTo class:TMale2;13.5:call LineOfSightTo class:TMale2;14:call LineOfSightTo class:TMale2` | `WarpZoneInfo.ActorEntered,WarpZoneInfo.ActorLeaving,WarpZoneInfo.Warp,WarpZoneInfo.UnWarp,ZoneInfo.ActorEntered,Pawn.Died` | 16 | one entry on `WarpZoneInfo2`, one `UnWarp` and one `Warp`, none on `WarpZoneInfo3`, no second entry within 5 s, the first probe after the warp within 12 units of (786.8, -446.5) |
| `zn-w2` | Unreal Tournament 436 | `DM-Fractal` | `5:Fire;7:call LineOfSightTo class:TMale2;7.5:call LineOfSightTo class:TMale2;8:place -577.6 -441.0 630.55;8:call LineOfSightTo class:TMale2;8.5:call LineOfSightTo class:TMale2;9:call LineOfSightTo class:TMale2;9.5:call LineOfSightTo class:TMale2;10:call LineOfSightTo class:TMale2;10.5:call LineOfSightTo class:TMale2;11:call LineOfSightTo class:TMale2;11.5:call LineOfSightTo class:TMale2;12:call LineOfSightTo class:TMale2;12.5:call LineOfSightTo class:TMale2;13:call LineOfSightTo class:TMale2;13.5:call LineOfSightTo class:TMale2;14:call LineOfSightTo class:TMale2` | `WarpZoneInfo.ActorEntered,WarpZoneInfo.ActorLeaving,WarpZoneInfo.Warp,WarpZoneInfo.UnWarp,ZoneInfo.ActorEntered,Pawn.Died` | 16 | the same on `WarpZoneInfo4` and `WarpZoneInfo5`, the column (-177.6, -449.0) |
| `zn-ctf1` | Unreal Tournament 436 | `CTF-Nucleus?game=Botpack.CTFGame` | `5:Fire;7.75:call LineOfSightTo class:TMale2;8:place 28.0 3516.0 444;8:call LineOfSightTo class:TMale2;8.25:call LineOfSightTo class:TMale2;8.5:call LineOfSightTo class:TMale2;8.75:call LineOfSightTo class:TMale2;9:call LineOfSightTo class:TMale2;9.25:call LineOfSightTo class:TMale2;9.5:call LineOfSightTo class:TMale2;9.75:call LineOfSightTo class:TMale2;10:call LineOfSightTo class:TMale2;10.25:call LineOfSightTo class:TMale2;10.5:call LineOfSightTo class:TMale2;10.75:call LineOfSightTo class:TMale2;11:call LineOfSightTo class:TMale2;11.25:call LineOfSightTo class:TMale2;11.5:call LineOfSightTo class:TMale2;11.75:call LineOfSightTo class:TMale2;12:call LineOfSightTo class:TMale2;12.25:call LineOfSightTo class:TMale2;12.5:call LineOfSightTo class:TMale2;12.75:call LineOfSightTo class:TMale2;13:call LineOfSightTo class:TMale2;13.25:call LineOfSightTo class:TMale2;13.5:call LineOfSightTo class:TMale2;13.75:call LineOfSightTo class:TMale2;14:call LineOfSightTo class:TMale2;14.25:call LineOfSightTo class:TMale2;14.5:call LineOfSightTo class:TMale2;14.75:call LineOfSightTo class:TMale2;15:call LineOfSightTo class:TMale2;15.25:call LineOfSightTo class:TMale2;15.5:call LineOfSightTo class:TMale2;15.75:call LineOfSightTo class:TMale2;16:call LineOfSightTo class:TMale2;16.25:call LineOfSightTo class:TMale2` | `Pawn.TakeDamage,Pawn.Died` | 40 | the centre settles at z 424 from 1.5 s after the placement |
| `zn-ctf2` | Unreal Tournament 436 | `CTF-Nucleus?game=Botpack.CTFGame` | `5:Fire;7.75:call LineOfSightTo class:TMale2;8:place 29.3 3505.2 444;8:call LineOfSightTo class:TMale2;8.25:call LineOfSightTo class:TMale2;8.5:call LineOfSightTo class:TMale2;8.75:call LineOfSightTo class:TMale2;9:call LineOfSightTo class:TMale2;9.25:call LineOfSightTo class:TMale2;9.5:call LineOfSightTo class:TMale2;9.75:call LineOfSightTo class:TMale2;10:call LineOfSightTo class:TMale2;10.25:call LineOfSightTo class:TMale2;10.5:call LineOfSightTo class:TMale2;10.75:call LineOfSightTo class:TMale2;11:call LineOfSightTo class:TMale2;11.25:call LineOfSightTo class:TMale2;11.5:call LineOfSightTo class:TMale2;11.75:call LineOfSightTo class:TMale2;12:call LineOfSightTo class:TMale2;12.25:call LineOfSightTo class:TMale2;12.5:call LineOfSightTo class:TMale2;12.75:call LineOfSightTo class:TMale2;13:call LineOfSightTo class:TMale2;13.25:call LineOfSightTo class:TMale2;13.5:call LineOfSightTo class:TMale2;13.75:call LineOfSightTo class:TMale2;14:call LineOfSightTo class:TMale2;14.25:call LineOfSightTo class:TMale2;14.5:call LineOfSightTo class:TMale2;14.75:call LineOfSightTo class:TMale2;15:call LineOfSightTo class:TMale2;15.25:call LineOfSightTo class:TMale2;15.5:call LineOfSightTo class:TMale2;15.75:call LineOfSightTo class:TMale2;16:call LineOfSightTo class:TMale2;16.25:call LineOfSightTo class:TMale2` | `Pawn.TakeDamage,Pawn.Died` | 40 | the centre settles at z 424 from 1.5 s after the placement |
| `zn-ctf3` | Unreal Tournament 436 | `CTF-Nucleus?game=Botpack.CTFGame` | `5:Fire;7.75:call LineOfSightTo class:TMale2;8:place -16.0 3504.0 444;8:call LineOfSightTo class:TMale2;8.25:call LineOfSightTo class:TMale2;8.5:call LineOfSightTo class:TMale2;8.75:call LineOfSightTo class:TMale2;9:call LineOfSightTo class:TMale2;9.25:call LineOfSightTo class:TMale2;9.5:call LineOfSightTo class:TMale2;9.75:call LineOfSightTo class:TMale2;10:call LineOfSightTo class:TMale2;10.25:call LineOfSightTo class:TMale2;10.5:call LineOfSightTo class:TMale2;10.75:call LineOfSightTo class:TMale2;11:call LineOfSightTo class:TMale2;11.25:call LineOfSightTo class:TMale2;11.5:call LineOfSightTo class:TMale2;11.75:call LineOfSightTo class:TMale2;12:call LineOfSightTo class:TMale2;12.25:call LineOfSightTo class:TMale2;12.5:call LineOfSightTo class:TMale2;12.75:call LineOfSightTo class:TMale2;13:call LineOfSightTo class:TMale2;13.25:call LineOfSightTo class:TMale2;13.5:call LineOfSightTo class:TMale2;13.75:call LineOfSightTo class:TMale2;14:call LineOfSightTo class:TMale2;14.25:call LineOfSightTo class:TMale2;14.5:call LineOfSightTo class:TMale2;14.75:call LineOfSightTo class:TMale2;15:call LineOfSightTo class:TMale2;15.25:call LineOfSightTo class:TMale2;15.5:call LineOfSightTo class:TMale2;15.75:call LineOfSightTo class:TMale2;16:call LineOfSightTo class:TMale2;16.25:call LineOfSightTo class:TMale2` | `Pawn.TakeDamage,Pawn.Died` | 40 | the centre passes z 384 and settles at or below z 100 |

## Regression sample

Fifteen stock maps on the fixed binary, 60 seconds each, as the committed sweeps ran them (`--url=<map>` plus the class pin on the Unreal Gold maps, the bot-free config root on the Unreal Tournament maps, and the `?game=` options the committed sweep used for `AS-Frigate`, `AS-Overlord` and `CTF-Face`), judged with the sweep runner's own `parse_log` and `classify`. Every cell reads committed / run. `script_warnings` is shown and not compared: Dig printed 28 where the committed row has 10, and Harobed and Chizra, whose three Stage runs differ from each other as the creatures' behaviour does, printed 6 to 12 and 2 to 17 against 15 and 17. The five other fields equal the committed row on every map (the three flock maps in all three of their runs).

| Game | Map | Verdict | Script errors | Exceptions | Missing natives | Stub hits | Script warnings |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| Unreal Gold | Vortex2 | pass / pass | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 |
| Unreal Gold | Dig | pass / pass | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 | 10 / 28 |
| Unreal Gold | NyLeve | pass / pass | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0,0,0 (3 runs) |
| Unreal Gold | Chizra | pass / pass | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 | 17 / 17,9,2 (3 runs) |
| Unreal Gold | Harobed | pass / pass | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 | 15 / 12,9,6 (3 runs) |
| Unreal Tournament | AS-Frigate | pass / pass | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 |
| Unreal Tournament | DM-Deck16][ | pass / pass | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 |
| Unreal Tournament | AS-Overlord | pass / pass | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 |
| Unreal Tournament | DM-Codex | pass / pass | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 |
| Unreal Tournament | CTF-Face | pass / pass | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 |
| Deus Ex | 00_Training | pass / pass | 0 / 0 | 0 / 0 | 0 / 0 | 1 / 1 | 0 / 0 |
| Deus Ex | 01_NYC_UNATCOIsland | pass / pass | 0 / 0 | 0 / 0 | 0 / 0 | 1 / 1 | 0 / 0 |
| Deus Ex | 02_NYC_BatteryPark | pass / pass | 0 / 0 | 0 / 0 | 0 / 0 | 1 / 1 | 0 / 0 |
| Deus Ex | 03_NYC_UNATCOHQ | pass / pass | 0 / 0 | 0 / 0 | 0 / 0 | 1 / 1 | 0 / 0 |
| Deus Ex | 04_NYC_Hotel | pass / pass | 0 / 0 | 0 / 0 | 0 / 0 | 1 / 1 | 0 / 0 |


The one fall death among the twelve single-run maps is `Trashbag1` (class `Trashbag2`, never traced in a physics mode) in 02_NYC_BatteryPark at 0.053 s, and a run of the previous binary has the same death at 0.054 s: not shown to differ.

The botmatch guard: DM-Deck16][ with four bots pinned in a config root of its own (`MinPlayers=0`, `InitialBots=4`) and the match started at 5 s, 300 s, traced for `GameInfo.Killed` and `Bot.PostBeginPlay` (four lines, the four bots), gave 31 kills against a floor of 9; the player died once; `script_errors`, `stub_hits`, `missing_natives` and `exceptions` were 0 and `script_warnings` 7.
