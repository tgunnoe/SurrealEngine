# Unreal Gold 226b sweep triage

Non-clean maps: 2 of 98. A map is clean only when it passes, loaded
the requested map, rendered, and logged no error line and no
gameplay-class stub. Every non-clean map has a section. `SCRIPT_WARNING`
lines are counted in each section and do not by themselves make a map
non-clean. `fps_avg` is recorded per row in sweep.json, and a row below 50
is weak evidence of absence: a map that rendered few frames had little
chance to log an error or a stub.

Evidence for a row is its verdict line, `maps_loaded`, failing lines and
warnings line (sweep.json, errors.json) and its stub list (stubs.json).

## NagomiSun

- verdict: fail, exit code 0
- maps_loaded: ["NagomiSun"]
- failing lines (first 1 distinct, log order):
  - `SCRIPT_ERROR Accessed None  Script call stack: at PathNodeIterator.BuildPath line 16 at Predator.Hiding.PickDestination line 919: pni.BuildPath(Location, home.Location) at Predator.Hiding line 1349: PickDestination()`
- warnings: 10
- gameplay stubs: `PathNodeIterator.BuildPath()`, `PathNodeIterator.GetFirst()`, `PathNodeIterator.GetLast()`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-unreal/NagomiSun.log --url=NagomiSun "<game folder>"`
- owner: unattributed (SCRIPT_ERROR: Accessed None  Script call stack: at PathNodeIterator.BuildPath line 16 at Preda); unimplemented natives, SurrealEngine/Packages/UPak/UPathNodeIterator.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: none applies; Docs/Status.md line 55 (Unreal (Gold): '226 version of the game launches. Menu options works most of the time. Single player maps can be played, as well as botmatches. Bots can sometimes move around and attack.')
- reproduced: flaky (second run exit code 0; run1 first line `SCRIPT_ERROR Accessed None  Script call stack: at PathNodeIterator.BuildPath line 16 at Predator.Hiding.PickDestination line 919: pni.BuildPath(Location, home.Location) at Predator.Hiding line 1349: P`; run2 first line `None`)

## Nevec

- verdict: pass, exit code 0
- maps_loaded: ["Nevec"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- warnings: 19
- gameplay stubs: `PathNodeIterator.BuildPath()`, `PathNodeIterator.GetFirst()`, `PathNodeIterator.GetLast()`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-unreal/Nevec.log --url=Nevec "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Packages/UPak/UPathNodeIterator.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 55 (Unreal (Gold): '226 version of the game launches. Menu options works most of the time. Single player maps can be played, as well as botmatches. Bots can sometimes move around and attack.')
- reproduced: yes (second run exit code 0; run1 first line `stubs:PathNodeIterator.BuildPath(),PathNodeIterator.GetFirst(),PathNodeIterator.GetLast()`; run2 first line `stubs:PathNodeIterator.BuildPath(),PathNodeIterator.GetFirst(),PathNodeIterator.GetLast()`)
