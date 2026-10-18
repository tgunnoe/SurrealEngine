# Deus Ex 1112fm sweep summary

Maps: 88. clean: 0. fail: 4. crash: 1. timeout: 0. pass-verdict but non-clean (stub-only): 83. warnings: not recorded.

Check: the verdict counts are recomputed from sweep.json by check_triage.py,
which exits 0 only if they equal the line above.

## Failure signatures

| Signature | Maps | Affected |
| :--- | ---: | :--- |
| stub Actor.AIEndEvent | 88 | 88 maps (see TRIAGE.md) |
| stub LevelInfo.InitEventManager | 88 | 88 maps (see TRIAGE.md) |
| stub Actor.AIClearEventCallback | 85 | 85 maps (see TRIAGE.md) |
| stub PlayerPawn.ResetKeyboard | 85 | 85 maps (see TRIAGE.md) |
| stub Actor.AISetEventCallback | 83 | 83 maps (see TRIAGE.md) |
| stub Actor.AISendEvent | 81 | 81 maps (see TRIAGE.md) |
| stub Actor.PlayBlendAnim | 75 | 75 maps (see TRIAGE.md) |
| stub Pawn.AIPickRandomDestination_Deus | 71 | 71 maps (see TRIAGE.md) |
| stub Actor.AIStartEvent | 52 | 52 maps (see TRIAGE.md) |
| stub Pawn.AICanSee | 17 | 17 maps (see TRIAGE.md) |
| stub Pawn.AIDirectionReachable | 10 | 10 maps (see TRIAGE.md) |
| no render | 4 | 06_HongKong_TongBase, 06_HongKong_WanChai_Canal, 09_NYC_ShipBelow, 09_NYC_ShipFan |
| verdict fail | 4 | 06_HongKong_TongBase, 06_HongKong_WanChai_Canal, 08_NYC_Street, 09_NYC_ShipFan |
| login: Failed to spawn player actor | 3 | 06_HongKong_TongBase, 06_HongKong_WanChai_Canal, 09_NYC_ShipFan |
| crash: FATAL signal 11 | 1 | 09_NYC_ShipBelow |
| script: Iterator statement without an iterator (ReachablePathnodes) | 1 | 08_NYC_Street |
| stub Pawn.ReachablePathnodes | 1 | 08_NYC_Street |
| verdict crash | 1 | 09_NYC_ShipBelow |

## Ranked fix candidates

Fully unblocks means every non-clean reason of the map is covered by the
candidate. Partial means the map loses one reason but keeps others.

| Rank | Candidate | Fully unblocks | Partial |
| ---: | :--- | ---: | ---: |
| 1 | implement every gameplay stub in SurrealEngine/Native/NActor.cpp | 0 | 88 |
| 2 | implement every gameplay stub in SurrealEngine/Native/NLevelInfo.cpp | 0 | 88 |
| 3 | implement every gameplay stub in SurrealEngine/Native/NPlayerPawn.cpp | 0 | 85 |
| 4 | implement every gameplay stub in SurrealEngine/Native/NPawn.cpp | 0 | 72 |
| 5 | fix signature: no render | 0 | 4 |
| 6 | fix signature: verdict fail | 0 | 4 |
| 7 | fix signature: login: Failed to spawn player actor | 0 | 3 |
| 8 | fix signature: crash: FATAL signal 11 | 0 | 1 |
| 9 | fix signature: script: Iterator statement without an iterator (ReachablePathnodes) | 0 | 1 |
| 10 | fix signature: verdict crash | 0 | 1 |

Fixing every file above together unblocks every stub-only map; the
three login failures, the iterator failure and the segfault each
need their own fix on top.
