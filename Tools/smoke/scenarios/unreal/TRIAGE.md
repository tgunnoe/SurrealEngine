# Unreal Gold 226b sweep triage

Non-clean maps: 1 of 55. A map is clean only when it passes, loaded
the requested map, rendered, and logged no error line and no
gameplay-class stub. Every non-clean map has a section. `SCRIPT_WARNING`
lines are counted in each section and do not by themselves make a map
non-clean. `fps_avg` is recorded per row in sweep.json, and a row below 50
is weak evidence of absence: a map that rendered few frames had little
chance to log an error or a stub.

Evidence for a row is its verdict line, `maps_loaded`, failing lines and
warnings line (sweep.json, errors.json) and its stub list (stubs.json).

## ExtremeLab

- verdict: crash, exit code -6
- maps_loaded: ["ExtremeLab"]
- failing lines (first 1 distinct, log order):
  - `FATAL signal 6`
- warnings: 0
- gameplay stubs: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-unreal/ExtremeLab.log --url=ExtremeLab "<game folder>"`
- owner: unknown: the row's recorded error lines hold only the signal line (no script error, missing native, exception or terminate message before it), so the aborting frame is not identified
- Status.md: none applies
- reproduced: yes (second run exit code -6; run1 first line `FATAL signal 6`; run2 first line `FATAL signal 6`)
