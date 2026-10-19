# UT 436 sweep triage

Non-clean maps: 0 of 83. A map is clean only when it passes, loaded
the requested map, rendered, and logged no error line and no
gameplay-class stub. Every non-clean map has a section. `SCRIPT_WARNING`
lines are counted in each section and do not by themselves make a map
non-clean. The sweep used no bots and no fire press, so `match_started`
is not counted. `fps_avg` is recorded per row in sweep.json, and a row
below 50 is weak evidence of absence: a map that rendered few frames had
little chance to log an error or a stub.

Evidence for a row is its verdict line, `maps_loaded`, failing lines and
warnings line (sweep.json, errors.json) and its stub list (stubs.json).

