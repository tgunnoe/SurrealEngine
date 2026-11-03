# Render runs

These runs drive stock maps of Unreal Gold 226b, Unreal Tournament 436 and Deus Ex 1112fm to look at four things the render code gets wrong or the status document lists as buggy: mirrors, the Nali seed and ASMD pickups, the screen flash and the statistics overlay in Deus Ex. Each run is an unattended smoke run. The picture is taken from outside the engine: each run that takes pictures is launched inside its own nested compositor (cage), and a picture is a screenshot of that compositor's whole 1280x720 output, so that windows of other applications on the desktop do not reach it. The only machine reading of the render path is the frame rate in the engine's smoke log. This file records the placements, the runs and what the logs show. It does not say what the pictures show: that is judged by readers who see the capture from the previous binary and the one from the fixed binary side by side, and the status document says what they found.

In the tables, `Previous binary` is the engine built from the source before the render changes and `Fixed binary` the engine built from the source after them. Captures are not committed.

## Recipe

Every run is started with the same flags, no `--user-dir`, and the host's settings copied (never edited) into a scratch configuration directory, so that every run uses the OpenGL device. A run that takes pictures runs under cage, which writes its session variables to a file before it starts the engine; the regression runs take none and run on the desktop session:

```
cage -- env SURREALWIDGETS_DISPLAY_BACKEND=X11 SurrealEngine --autostart --exit-after=<seconds> \
    --smoke-log=<log> --url=<map> --exec='<items>' [--trace-script=<functions>] "<game folder>"
```

Tools/smoke/README.md describes the flags, `place` and `call`. The flash runs add `--trace-script=Pawn.HeadZoneChange` (the pool) or `--trace-script=PlayerPawn.ClientAdjustGlow` (the Invisibility), and the botmatch adds the kill traces of its row. What these runs add to them:

- The player is put at each point with `place <x> <y> <z> <yaw>`, a few seconds after login (Unreal Tournament starts its match with the console `Fire`, five seconds after login, as the zone runs do). The `PLACE` line of the smoke log gives the location asked for and the one the engine left the player at; a run whose two locations are more than 64 units apart is repeated at an alternate point of the same placement, and the table of placements lists the point each run used.
- A capture is a `grim` shot of the whole output of that cage, 1280x720, at fixed delays after the `PLACE` line. A capture counts only when the engine's window exists on the X server of that cage, the image is 1280x720, the picture is not uniform (so not black), and the capture before the move differs from the captures after it (the first capture of the overlay and Invisibility runs takes no part in that check, since the picture is expected to stay the same there).
- The frame rate is read from the `FPS` lines after the move, leaving out the first of them; the overlay run has no move, and its lines are those after login, leaving out the first two. A run with a line below 1.0 is a host stall and is repeated.
- The flash is looked at in two situations: under the water of a pool in NyLeve, and in Vortex2 with an Invisibility picked up and active (`summon UnrealI.Invisibility`, a short walk into the item, `ActivateItem`, then `place` back at the start point), which holds a constant glow on any map.
- Every point was computed from the map package: the centre and normal of a mirror surface, the position of a pickup, the water surface of a pool. The shift the engine made is in the table of the pairs.

The three calibration runs, one per game on the previous binary, gave 1280x720 pictures that were all attributed to the engine. The delays after a move are 3 s and 6 s for Unreal Gold and Deus Ex and 5 s and 8 s for Unreal Tournament, whose picture was still changing 3 to 5 s after the move. The engine logged in 0.8 to 1.1 s after the start of a run and the first `PLACE` line came 6.9 to 9.9 s after the start. Cage ran with its headless backend (`WLR_BACKENDS=headless`), because the desktop of the machine had no active display output at the time; the picture is the same 1280x720 output. A census of the vec4 inequality, made by building a copy of the previous source with the operator deleted, gave 8 errors at 2 sites, the flash conditions of the OpenGL and Vulkan devices; the Direct3D 11 device is not built on the machine.

## Placements

| Id | Game | Map | Placement (x y z yaw) | Looks at |
| :--- | :--- | :--- | :--- | :--- |
| m-chizra | Unreal Gold | Chizra | `place -9586 -3976 -240 45` | the one x-facing mirror of Unreal Gold |
| m-terralift | Unreal Gold | TerraLift | `place -654 -224 3520 15` | two mirror planes 16 units apart in one view |
| m-cinder | Unreal Tournament | DOM-Cinder | `place -497 -175 -80 45` | a y-wall mirror; the second mirror is not in view |
| m-hotel | Deus Ex | 02_NYC_Hotel | `place 1797 -1226 112 135` | the x-wall mirror of the hotel room |
| m-wanchai | Deus Ex | 06_HongKong_WanChai_Street | `place 1226 -1583 1712 300` | two mirror planes at right angles |
| m-training | Deus Ex | 00_Training | `place 1494 834 -100 345` | a hall of 19 floor mirrors in view |
| m-skycaves | Unreal Gold | SkyCaves | `place 352 -5856 1184 60` | a platform of floor mirrors, 11 in view |
| k-bluff-seeds | Unreal Gold | Bluff | `place 666 13 -3904 23` | the Nali seeds of Bluff |
| k-bluff-asmd | Unreal Gold | Bluff | `place 587 -289 -3216 90` | the ASMD of Bluff |
| k-niven-asmd0 | Unreal Tournament | CTF-Niven | `place -2316 893 -230 180` | the first ASMD of CTF-Niven |
| k-niven-asmd1 | Unreal Tournament | CTF-Niven | `place 766 -773 -208 180` | the second ASMD of CTF-Niven |
| f-nyleve | Unreal Gold | NyLeve | `place -2309 -15808 -6349 0` | under the water of the NyLeve pool, 80 units below the surface |
| f-invis | Unreal Gold | Vortex2 | `place -2135 -89 -384 128` | the Vortex2 start point, with an Invisibility active |

## Mirrors, pickups, flash and overlay

| Pair | Previous binary | Fixed binary | Shift of the move (previous, fixed) | Lowest FPS after the move (previous, fixed) | Probe lines (previous, fixed) |
| :--- | :--- | :--- | :--- | :--- | :--- |
| mir-chizra | mir-chizra-base | mir-chizra-node | 0.0, 0.0 | 311.4, 331.9 | 2, 2 |
| mir-terralift | mir-terralift-base | mir-terralift-node | 39.0, 39.0 | 459.8, 366.1 | 2, 2 |
| mir-cinder | mir-cinder-base | mir-cinder-node-r2 | 0.0, 0.0 | 656.0, 552.4 | 2, 2 |
| mir-hotel | mir-hotel-base | mir-hotel-node | 0.0, 0.0 | 802.3, 683.5 | 2, 2 |
| mir-wanchai | mir-wanchai-base | mir-wanchai-node | 0.0, 0.0 | 315.0, 244.6 | 2, 2 |
| mir-training | mir-training-base | mir-training-node | 0.0, 0.0 | 259.0, 120.3 | 2, 2 |
| mir-skycaves | mir-skycaves-base | mir-skycaves-node | 39.0, 39.0 | 486.9, 387.4 | 2, 2 |
| pick-bluff | pick-bluff-base | pick-bluff-node | 0.0 / 0.0, 0.0 / 0.0 | 458.8, 474.3 | 2, 2 |
| flash-nyleve | flash-nyleve-base | flash-nyleve-node | 0.0, 0.0 | 1185.5, 1094.1 | 1, 1 |
| flash-invis | flash-invis-base | flash-invis-node | 0.0, 0.0 | 498.1, 485.7 | 1, 1 |
| ovl-training | ovl-training-base | ovl-training-node | -, - | 308.3, 283.7 | 1, 1 |

## Regression sample

| Map | Verdict | Script errors | Exceptions | Missing natives | Stub hits | Warnings (run, committed) |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| Vortex2 | MATCH | 0 | 0 | 0 | 0 | 0, 0 |
| Dig | MATCH | 0 | 0 | 0 | 0 | 42, 10 |
| NyLeve | MATCH | 0 | 0 | 0 | 0 | 0, 0 |
| Chizra | MATCH | 0 | 0 | 0 | 0 | 23, 17 |
| Harobed | MATCH | 0 | 0 | 0 | 0 | 12, 15 |
| AS-Frigate | MATCH | 0 | 0 | 0 | 0 | 0, 0 |
| DM-Deck16][ | MATCH | 0 | 0 | 0 | 0 | 0, 0 |
| AS-Overlord | MATCH | 0 | 0 | 0 | 0 | 0, 0 |
| DM-Codex | MATCH | 0 | 0 | 0 | 0 | 0, 0 |
| CTF-Face | MATCH | 0 | 0 | 0 | 0 | 0, 0 |
| 00_Training | MATCH | 0 | 0 | 0 | 1 | 0, 0 |
| 01_NYC_UNATCOIsland | MATCH | 0 | 0 | 0 | 1 | 0, 0 |
| 02_NYC_BatteryPark | MATCH | 0 | 0 | 0 | 1 | 0, 0 |
| 03_NYC_UNATCOHQ | MATCH | 0 | 0 | 0 | 1 | 0, 0 |
| 04_NYC_Hotel | MATCH | 0 | 0 | 0 | 1 | 0, 0 |

The botmatch (Unreal Tournament DM-Deck16][, ten bots at difficulty 3: the URL asks for four and the trace names ten, 300 s, `Fire` at 5 s) logged 109 lines of `GameInfo.Killed:`, which is not comparable with the 18 of the earlier measurement; the player died once, at 86.1 s, to a bot; the counters of its `SUMMARY` line are 0 script errors, 0 stub hits, 0 missing natives, 0 exceptions and 6 script warnings at a mean of 960.5 `FPS`.

## What the runs do not cover

The pictures are judged outside this file. The fixed binary has not been compared with the original games. The Direct3D 11 device is not built on the machine of these runs. A frame rate above 60 on this machine says nothing about slower ones, and inside the nested compositor the frame rate is lower than on the desktop session. The Deus Ex pictures are 1280x720 and are framed differently from the 1280x1024 pictures of the desktop session.

The CTF-Niven ASMD pair was not observed: with the game type `Botpack.CTFGame` the level has no actor named `ASMD0` or `ASMD1`, so the pickups were not there to look at, and the player died in both runs of the pair. The Unreal Tournament pair of DOM-Cinder needed a third attempt of the fixed-binary run because the player died in the first two.

The flash runs cannot show the flash change. At the frame rates of these runs (485 to 1185 `FPS`) the stock view-flash script sends the device no flash: one easing step from rest is snapped back to rest on every frame, and a step escapes the snap only below about 41 `FPS` for the tint of water, 79 for Invisibility and 205 for a heavy hit flash.
