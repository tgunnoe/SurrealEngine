# Unreal Gold 226b sweep summary

Maps: 98. clean: 96 of 98. fail: 1. crash: 0. timeout: 0. pass-verdict but non-clean: 1. warnings: 270.

Check: the verdict counts are recomputed from sweep.json by the `check`
command of the triage module, which exits 0 only if they equal the line
above.

## Warning sites

| Site | Lines | Maps |
| :--- | ---: | ---: |
| Accessed None in HorseFly.meander | 200 | 14 |
| Accessed None in SinglePlayer.Killed | 16 | 7 |
| Accessed None in Predator.PickDestination | 10 | 1 |
| Accessed None in NaliPriest.Killed | 9 | 3 |
| Accessed None in Nali.Killed | 8 | 2 |
| Accessed None in WarpZoneMarker.PostBeginPlay | 8 | 1 |
| Accessed None in UPakShieldEffect.Tick | 6 | 2 |
| Accessed None in SinglePlayer.ScoreKill | 5 | 2 |
| Accessed None in UPakSinglePlayer.Killed | 3 | 1 |
| Accessed None in BabyCow.PickDestination | 2 | 2 |
| Accessed None in CrashSiteGame.Killed | 1 | 1 |
| Accessed None in Krall.MeleeDamageTarget | 1 | 1 |
| Accessed None in UPakTransitionInfo.ScoreKill | 1 | 1 |

## Failure signatures

| Signature | Maps | Affected |
| :--- | ---: | :--- |
| stub PathNodeIterator.BuildPath() | 2 | NagomiSun, Nevec |
| stub PathNodeIterator.GetFirst() | 2 | NagomiSun, Nevec |
| stub PathNodeIterator.GetLast() | 2 | NagomiSun, Nevec |
| SCRIPT_ERROR: Accessed None  Script call stack: at PathNodeIterator.BuildPath line 16 at Preda | 1 | NagomiSun |
| verdict fail | 1 | NagomiSun |

## Ranked fix candidates

Fully unblocks means every non-clean reason of the map is covered by the
candidate. Partial means the map loses one reason but keeps others.

| Rank | Candidate | Fully unblocks | Partial |
| ---: | :--- | ---: | ---: |
| 1 | implement every gameplay stub in SurrealEngine/Packages/UPak/UPathNodeIterator.cpp | 1 | 1 |
| 2 | fix signature: SCRIPT_ERROR: Accessed None  Script call stack: at PathNodeIterator.BuildPath line 16 at Preda | 0 | 1 |
| 3 | fix signature: verdict fail | 0 | 1 |

A map becomes clean only when every signature that lists it above is fixed.
