# Unreal Gold 226b sweep summary (60 s per map, 55 maps)

| Outcome | Maps |
| :--- | ---: |
| clean (pass, requested map loaded, fps recorded, no stubs) | 46 |
| fail | 9 |
| crash | 0 |
| timeout | 0 |
| idle (pass without the map loaded or without fps) | 0 |

Check: the counts are the verdict tally of `sweep.json`
(`python3 -I -c 'import json,collections;print(collections.Counter(m["verdict"] for m in json.load(open("sweep.json"))["maps"]))'`
gives pass 46, fail 9), and `check_triage.py` derives the same 9
non-clean maps (`non-clean: 9 of 55`). All 9 failing rows exit 0 with a
`SUMMARY` line; they fail only because `script_errors` is above zero.
No map logged a `STUB`, `MISSING_NATIVE`, `EXCEPTION` or `FATAL` line.
`fps_avg` was not judged (three sweeps shared one display).

## Failure signatures

| Signature | Maps |
| :--- | :--- |
| `Accessed None` from `Object.SubtractEqual_FloatFloat` at `SinglePlayer.Killed` line 20 (`Killer.PlayerReplicationInfo.Score`) and `GameInfo.ScoreKill` line 1001 (`Other.PlayerReplicationInfo.Score`), reached from `Pawn.Died` | Bluff, Ceremony, Chizra, Dig, Dug, SkyCaves, SkyTown, SpireVillage, TheSunspire (9) |

## Ranked candidate fix work

1. Make the engine report a None access uniformly, then let the project
   owner rule whether a faithful `Accessed None` counts against clean.
   Mechanism: `SubtractEqual_FloatFloat` receives a None out-parameter,
   `ExpressionValue::CheckType` throws "Accessed None"
   (`SurrealEngine/VM/ExpressionValue.h`), and the catch in
   `Frame::CallNative` (`SurrealEngine/VM/Frame.cpp`) logs it as
   "Script error: ", which the harness counts. The same condition in a
   statement or call target logs an unprefixed line the harness never
   sees. The None is reached by stock script (monsters and Nali have no
   replication info), so it is not a map fault. Resolving either side
   unblocks all 9 maps.

No other work is needed for the other 46 maps by this sweep.
