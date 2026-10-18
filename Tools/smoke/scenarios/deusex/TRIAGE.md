# Deus Ex 1112fm sweep triage

Non-clean maps: 2 of 88. A map is clean only when it passes, loaded
the requested map, rendered, and logged no error line and no
gameplay-class stub. Every non-clean map has a section. Other engine
runs shared the display during this sweep, so `fps_avg` is not evidence.

Evidence for a stub-only row is its stub list (stubs.json) and its
exit code and SUMMARY-bearing pass verdict (sweep.json).

## 11_Paris_Underground

- verdict: crash, exit code -11
- maps_loaded: ["11_Paris_Underground"]
- failing lines (first 1 distinct, log order):
  - `FATAL signal 11`
- warnings: 0
- gameplay stubs: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/11_Paris_Underground.log --url=11_Paris_Underground "<game folder>"`
- owner: unknown native or VM subsystem: the logs show SIGSEGV after LOGIN with no SCRIPT_ERROR, MISSING_NATIVE or EXCEPTION line before it, and the map did not crash when re-run alone, so the faulting frame is not identified
- Status.md: none applies
- reproduced: no (second run exit code 0; run1 first line `FATAL signal 11`; run2 first line `None`)

## 12_Vandenberg_Cmd

- verdict: crash, exit code -11
- maps_loaded: ["12_Vandenberg_Cmd"]
- failing lines (first 1 distinct, log order):
  - `FATAL signal 11`
- warnings: 22174
- gameplay stubs: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/12_Vandenberg_Cmd.log --url=12_Vandenberg_Cmd "<game folder>"`
- owner: unknown native or VM subsystem: the logs show SIGSEGV after LOGIN with no SCRIPT_ERROR, MISSING_NATIVE or EXCEPTION line before it, and the map did not crash when re-run alone, so the faulting frame is not identified
- Status.md: none applies
- reproduced: no (second run exit code 0; run1 first line `FATAL signal 11`; run2 first line `None`)
