# Deus Ex 1112fm sweep triage

Non-clean maps: 0 of 88. A map is clean only when it passes, loaded
the requested map, rendered, and logged no error line and no
gameplay-class stub. Every non-clean map has a section. This run was made
with no other engine, build or sweep on the host; `fps_avg` is recorded per
row in sweep.json, and a row below 50 is weak evidence of absence: a map
that rendered few frames had little chance to log an error or a stub.

Evidence for a stub-only row is its stub list (stubs.json) and its
exit code and SUMMARY-bearing pass verdict (sweep.json).
