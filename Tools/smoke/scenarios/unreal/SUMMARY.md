# Unreal Gold 226b sweep summary

Maps: 55. clean: 54 of 55. fail: 0. crash: 1. timeout: 0. pass-verdict but non-clean: 0. warnings: 125.

Check: the verdict counts are recomputed from sweep.json by the `check`
command of the triage module, which exits 0 only if they equal the line
above.

## Warning sites

| Site | Lines | Maps |
| :--- | ---: | ---: |
| Accessed None in HorseFly.meander | 94 | 9 |
| Accessed None in SinglePlayer.Killed | 18 | 7 |
| Accessed None in SinglePlayer.ScoreKill | 8 | 4 |
| Accessed None in Slith.TakeDamage | 2 | 1 |
| Accessed None in BabyCow.PickDestination | 1 | 1 |
| Accessed None in PlayerStart.PlayTeleportEffect | 1 | 1 |
| Accessed None in SkaarjBerserker.TakeDamage | 1 | 1 |

## Failure signatures

| Signature | Maps | Affected |
| :--- | ---: | :--- |
| crash: FATAL signal 6 | 1 | ExtremeLab |

## Ranked fix candidates

Fully unblocks means every non-clean reason of the map is covered by the
candidate. Partial means the map loses one reason but keeps others.

| Rank | Candidate | Fully unblocks | Partial |
| ---: | :--- | ---: | ---: |
| 1 | fix signature: crash: FATAL signal 6 | 1 | 0 |

A map becomes clean only when every signature that lists it above is fixed.
