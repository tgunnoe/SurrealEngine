# Triage: non-clean maps

Every one of the 72 sections below has the same single cause: the log line
`STUB GetNetworkNumber`, a stub hit in a gameplay class (`GameInfo`). No
row has a script error, missing native, exception or fatal line, a
non-`pass` verdict, an idle run, or a map change. Frame rate is not
evidence in this sweep (three sweeps shared one display).

Common to all sections: error lines: none (the extractor recorded no
`SCRIPT_ERROR`, `MISSING_NATIVE`, `EXCEPTION` or `FATAL` line; see
`errors.json`). Owner: `GameInfo.GetNetworkNumber`, registered at
`SurrealEngine/Native/NGameInfo.cpp:12` and implemented at line 27, which
returns "0" and logs the stub. Upstream `Docs/Status.md` has no matching item.

## AS-Frigate

- verdict: pass, exit code 0
- maps_loaded: ["AS-Frigate"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/AS-Frigate.log --url=AS-Frigate "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: yes (second run exit code 0, same gameplay stub set, no other failing line)

## AS-Guardia

- verdict: pass, exit code 0
- maps_loaded: ["AS-Guardia"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/AS-Guardia.log --url=AS-Guardia "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: yes (second run exit code 0, same gameplay stub set, no other failing line)

## AS-HiSpeed

- verdict: pass, exit code 0
- maps_loaded: ["AS-HiSpeed"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/AS-HiSpeed.log --url=AS-HiSpeed "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: yes (second run exit code 0, same gameplay stub set, no other failing line)

## AS-Mazon

- verdict: pass, exit code 0
- maps_loaded: ["AS-Mazon"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/AS-Mazon.log --url=AS-Mazon "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: yes (second run exit code 0, same gameplay stub set, no other failing line)

## AS-OceanFloor

- verdict: pass, exit code 0
- maps_loaded: ["AS-OceanFloor"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/AS-OceanFloor.log --url=AS-OceanFloor "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: yes (second run exit code 0, same gameplay stub set, no other failing line)

## AS-Overlord

- verdict: pass, exit code 0
- maps_loaded: ["AS-Overlord"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/AS-Overlord.log --url=AS-Overlord "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## AS-Rook

- verdict: pass, exit code 0
- maps_loaded: ["AS-Rook"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/AS-Rook.log --url=AS-Rook "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## AS-Tutorial

- verdict: pass, exit code 0
- maps_loaded: ["AS-Tutorial"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/AS-Tutorial.log --url=AS-Tutorial "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## CTF-Command

- verdict: pass, exit code 0
- maps_loaded: ["CTF-Command"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/CTF-Command.log --url=CTF-Command "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## CTF-Coret

- verdict: pass, exit code 0
- maps_loaded: ["CTF-Coret"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/CTF-Coret.log --url=CTF-Coret "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## CTF-Cybrosis][

- verdict: pass, exit code 0
- maps_loaded: ["CTF-Cybrosis]["]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/CTF-Cybrosis][.log --url=CTF-Cybrosis][ "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## CTF-Darji16

- verdict: pass, exit code 0
- maps_loaded: ["CTF-Darji16"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/CTF-Darji16.log --url=CTF-Darji16 "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## CTF-Dreary

- verdict: pass, exit code 0
- maps_loaded: ["CTF-Dreary"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/CTF-Dreary.log --url=CTF-Dreary "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## CTF-EternalCave

- verdict: pass, exit code 0
- maps_loaded: ["CTF-EternalCave"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/CTF-EternalCave.log --url=CTF-EternalCave "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## CTF-Face

- verdict: pass, exit code 0
- maps_loaded: ["CTF-Face"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/CTF-Face.log --url=CTF-Face "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## CTF-Face][

- verdict: pass, exit code 0
- maps_loaded: ["CTF-Face]["]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/CTF-Face][.log --url=CTF-Face][ "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## CTF-Gauntlet

- verdict: pass, exit code 0
- maps_loaded: ["CTF-Gauntlet"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/CTF-Gauntlet.log --url=CTF-Gauntlet "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## CTF-HallOfGiants

- verdict: pass, exit code 0
- maps_loaded: ["CTF-HallOfGiants"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/CTF-HallOfGiants.log --url=CTF-HallOfGiants "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## CTF-High

- verdict: pass, exit code 0
- maps_loaded: ["CTF-High"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/CTF-High.log --url=CTF-High "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## CTF-Hydro16

- verdict: pass, exit code 0
- maps_loaded: ["CTF-Hydro16"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/CTF-Hydro16.log --url=CTF-Hydro16 "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## CTF-Kosov

- verdict: pass, exit code 0
- maps_loaded: ["CTF-Kosov"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/CTF-Kosov.log --url=CTF-Kosov "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## CTF-LavaGiant

- verdict: pass, exit code 0
- maps_loaded: ["CTF-LavaGiant"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/CTF-LavaGiant.log --url=CTF-LavaGiant "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## CTF-Niven

- verdict: pass, exit code 0
- maps_loaded: ["CTF-Niven"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/CTF-Niven.log --url=CTF-Niven "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## CTF-November

- verdict: pass, exit code 0
- maps_loaded: ["CTF-November"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/CTF-November.log --url=CTF-November "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## CTF-Noxion16

- verdict: pass, exit code 0
- maps_loaded: ["CTF-Noxion16"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/CTF-Noxion16.log --url=CTF-Noxion16 "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## CTF-Nucleus

- verdict: pass, exit code 0
- maps_loaded: ["CTF-Nucleus"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/CTF-Nucleus.log --url=CTF-Nucleus "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## CTF-Orbital

- verdict: pass, exit code 0
- maps_loaded: ["CTF-Orbital"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/CTF-Orbital.log --url=CTF-Orbital "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-Agony

- verdict: pass, exit code 0
- maps_loaded: ["DM-Agony"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-Agony.log --url=DM-Agony "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-ArcaneTemple

- verdict: pass, exit code 0
- maps_loaded: ["DM-ArcaneTemple"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-ArcaneTemple.log --url=DM-ArcaneTemple "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-Barricade

- verdict: pass, exit code 0
- maps_loaded: ["DM-Barricade"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-Barricade.log --url=DM-Barricade "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-Codex

- verdict: pass, exit code 0
- maps_loaded: ["DM-Codex"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-Codex.log --url=DM-Codex "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-Conveyor

- verdict: pass, exit code 0
- maps_loaded: ["DM-Conveyor"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-Conveyor.log --url=DM-Conveyor "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-Crane

- verdict: pass, exit code 0
- maps_loaded: ["DM-Crane"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-Crane.log --url=DM-Crane "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-Curse][

- verdict: pass, exit code 0
- maps_loaded: ["DM-Curse]["]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-Curse][.log --url=DM-Curse][ "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-Cybrosis][

- verdict: pass, exit code 0
- maps_loaded: ["DM-Cybrosis]["]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-Cybrosis][.log --url=DM-Cybrosis][ "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-Deck16][

- verdict: pass, exit code 0
- maps_loaded: ["DM-Deck16]["]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-Deck16][.log --url=DM-Deck16][ "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-Fetid

- verdict: pass, exit code 0
- maps_loaded: ["DM-Fetid"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-Fetid.log --url=DM-Fetid "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-Fractal

- verdict: pass, exit code 0
- maps_loaded: ["DM-Fractal"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-Fractal.log --url=DM-Fractal "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-Gothic

- verdict: pass, exit code 0
- maps_loaded: ["DM-Gothic"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-Gothic.log --url=DM-Gothic "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-Grinder

- verdict: pass, exit code 0
- maps_loaded: ["DM-Grinder"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-Grinder.log --url=DM-Grinder "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-HealPod][

- verdict: pass, exit code 0
- maps_loaded: ["DM-HealPod]["]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-HealPod][.log --url=DM-HealPod][ "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-HyperBlast

- verdict: pass, exit code 0
- maps_loaded: ["DM-HyperBlast"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-HyperBlast.log --url=DM-HyperBlast "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-KGalleon

- verdict: pass, exit code 0
- maps_loaded: ["DM-KGalleon"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-KGalleon.log --url=DM-KGalleon "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-Liandri

- verdict: pass, exit code 0
- maps_loaded: ["DM-Liandri"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-Liandri.log --url=DM-Liandri "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-Malevolence

- verdict: pass, exit code 0
- maps_loaded: ["DM-Malevolence"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-Malevolence.log --url=DM-Malevolence "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-Mojo][

- verdict: pass, exit code 0
- maps_loaded: ["DM-Mojo]["]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-Mojo][.log --url=DM-Mojo][ "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-Morbias][

- verdict: pass, exit code 0
- maps_loaded: ["DM-Morbias]["]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-Morbias][.log --url=DM-Morbias][ "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-Morpheus

- verdict: pass, exit code 0
- maps_loaded: ["DM-Morpheus"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-Morpheus.log --url=DM-Morpheus "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-Oblivion

- verdict: pass, exit code 0
- maps_loaded: ["DM-Oblivion"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-Oblivion.log --url=DM-Oblivion "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-Peak

- verdict: pass, exit code 0
- maps_loaded: ["DM-Peak"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-Peak.log --url=DM-Peak "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-Phobos

- verdict: pass, exit code 0
- maps_loaded: ["DM-Phobos"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-Phobos.log --url=DM-Phobos "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-Pressure

- verdict: pass, exit code 0
- maps_loaded: ["DM-Pressure"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-Pressure.log --url=DM-Pressure "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-Pyramid

- verdict: pass, exit code 0
- maps_loaded: ["DM-Pyramid"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-Pyramid.log --url=DM-Pyramid "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-Shrapnel][

- verdict: pass, exit code 0
- maps_loaded: ["DM-Shrapnel]["]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-Shrapnel][.log --url=DM-Shrapnel][ "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-SpaceNoxx

- verdict: pass, exit code 0
- maps_loaded: ["DM-SpaceNoxx"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-SpaceNoxx.log --url=DM-SpaceNoxx "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-Stalwart

- verdict: pass, exit code 0
- maps_loaded: ["DM-Stalwart"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-Stalwart.log --url=DM-Stalwart "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-StalwartXL

- verdict: pass, exit code 0
- maps_loaded: ["DM-StalwartXL"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-StalwartXL.log --url=DM-StalwartXL "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-Tempest

- verdict: pass, exit code 0
- maps_loaded: ["DM-Tempest"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-Tempest.log --url=DM-Tempest "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-Turbine

- verdict: pass, exit code 0
- maps_loaded: ["DM-Turbine"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-Turbine.log --url=DM-Turbine "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-Tutorial

- verdict: pass, exit code 0
- maps_loaded: ["DM-Tutorial"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-Tutorial.log --url=DM-Tutorial "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DM-Zeto

- verdict: pass, exit code 0
- maps_loaded: ["DM-Zeto"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DM-Zeto.log --url=DM-Zeto "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DOM-Cinder

- verdict: pass, exit code 0
- maps_loaded: ["DOM-Cinder"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DOM-Cinder.log --url=DOM-Cinder "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DOM-Condemned

- verdict: pass, exit code 0
- maps_loaded: ["DOM-Condemned"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DOM-Condemned.log --url=DOM-Condemned "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DOM-Cryptic

- verdict: pass, exit code 0
- maps_loaded: ["DOM-Cryptic"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DOM-Cryptic.log --url=DOM-Cryptic "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DOM-Gearbolt

- verdict: pass, exit code 0
- maps_loaded: ["DOM-Gearbolt"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DOM-Gearbolt.log --url=DOM-Gearbolt "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DOM-Ghardhen

- verdict: pass, exit code 0
- maps_loaded: ["DOM-Ghardhen"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DOM-Ghardhen.log --url=DOM-Ghardhen "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DOM-Lament

- verdict: pass, exit code 0
- maps_loaded: ["DOM-Lament"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DOM-Lament.log --url=DOM-Lament "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DOM-Leadworks

- verdict: pass, exit code 0
- maps_loaded: ["DOM-Leadworks"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DOM-Leadworks.log --url=DOM-Leadworks "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DOM-MetalDream

- verdict: pass, exit code 0
- maps_loaded: ["DOM-MetalDream"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DOM-MetalDream.log --url=DOM-MetalDream "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DOM-Olden

- verdict: pass, exit code 0
- maps_loaded: ["DOM-Olden"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DOM-Olden.log --url=DOM-Olden "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DOM-Sesmar

- verdict: pass, exit code 0
- maps_loaded: ["DOM-Sesmar"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DOM-Sesmar.log --url=DOM-Sesmar "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run

## DOM-Tutorial

- verdict: pass, exit code 0
- maps_loaded: ["DOM-Tutorial"]
- evidence: `STUB GetNetworkNumber` (stubs.json: owner GameInfo, gameplay true); `SUMMARY` stub_hits=1
- error lines: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut/DOM-Tutorial.log --url=DOM-Tutorial "/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY"`
- owner: `GameInfo.GetNetworkNumber` in `SurrealEngine/Native/NGameInfo.cpp` (native is a stub that returns "0")
reproduced: not-run
