# Unreal Gold 226b sweep summary

Maps: 98. clean: 97 of 98. fail: 0. crash: 0. timeout: 0. pass-verdict but non-clean: 1. warnings: 198.

Check: the verdict counts are recomputed from sweep.json by the `check`
command of the triage module, which exits 0 only if they equal the line
above.

## Warning sites

| Site | Lines | Maps |
| :--- | ---: | ---: |
| Accessed None in HorseFly.meander | 166 | 14 |
| Accessed None in SinglePlayer.Killed | 17 | 8 |
| Accessed None in UPakShieldEffect.Tick | 6 | 2 |
| Accessed None in SinglePlayer.ScoreKill | 5 | 2 |
| Accessed None in BabyCow.PickDestination | 2 | 2 |
| Accessed None in Slith.TakeDamage | 1 | 1 |
| Accessed None in UPakTransitionInfo.ScoreKill | 1 | 1 |

## Failure signatures

| Signature | Maps | Affected |
| :--- | ---: | :--- |
| stub PathNodeIterator.BuildPath() | 1 | Nevec |
| stub PathNodeIterator.GetFirst() | 1 | Nevec |
| stub PathNodeIterator.GetLast() | 1 | Nevec |

## Ranked fix candidates

Fully unblocks means every non-clean reason of the map is covered by the
candidate. Partial means the map loses one reason but keeps others.

| Rank | Candidate | Fully unblocks | Partial |
| ---: | :--- | ---: | ---: |
| 1 | implement every gameplay stub in SurrealEngine/Packages/UPak/UPathNodeIterator.cpp | 1 | 0 |

A map becomes clean only when every signature that lists it above is fixed.
