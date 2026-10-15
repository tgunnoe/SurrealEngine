# UT 436 sweep summary

60 seconds per map, 83 maps, no bots. Counts derive from `sweep.json` and the
sidecars; `check_triage.py` recomputes the clean set from the same files.

| Class | Maps |
| :--- | ---: |
| Clean | 11 |
| Not clean | 72 |
| Verdict fail | 0 |
| Verdict crash | 0 |
| Verdict timeout | 0 |
| Verdict pass | 83 |
| Idle (pass, requested map absent or null fps) | 0 |

All 72 non-clean maps are `pass` rows made non-clean by one gameplay
stub. Clean maps: CTF-Tutorial, CityIntro, EOL_Assault, EOL_CTF, EOL_Challenge, EOL_Deathmatch, EOL_Domination, EOL_Statues, Entry, UT-Logo-Map, UTCredits.

## Failure signatures

| Signature | Maps |
| :--- | ---: |
| `STUB GetNetworkNumber` (`GameInfo`) | 72 |

No other signature occurs in any log.

## Ranked fix work

1. Implement `GameInfo.GetNetworkNumber` (`SurrealEngine/Native/NGameInfo.cpp`):
   unblocks 72 maps (all of them, since it is the only reason any is
   non-clean). Partial unblocks: none.

## Check

`python3 -I Tools/smoke/scenarios/ut/check_triage.py` recomputes the non-clean
set (72) from `sweep.json`, `stubs.json`, `errors.json` and compares it with
the `TRIAGE.md` sections. Counts above were taken from `sweep.json` verdict
tallies and the gameplay flags in `stubs.json`.
