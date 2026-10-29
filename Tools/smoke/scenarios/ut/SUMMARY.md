# UT 436 sweep summary

Maps: 83. clean: 83 of 83. fail: 0. crash: 0. timeout: 0. pass-verdict but non-clean: 0. warnings: 4058.

Check: the verdict counts are recomputed from sweep.json by the `check`
command of the triage module, which exits 0 only if they equal the line
above.

## Warning sites

| Site | Lines | Maps |
| :--- | ---: | ---: |
| Accessed None in UnrealCTFScoreboard.DrawTrailer | 3177 | 1 |
| Accessed None in MinigunCannon.SeePlayer | 869 | 1 |
| Accessed None in UTConsole.AddMessage | 8 | 1 |
| Accessed None in TeamCannon.SeePlayer | 4 | 3 |

## Failure signatures

| Signature | Maps | Affected |
| :--- | ---: | :--- |
| none | | |

## Ranked fix candidates

Fully unblocks means every non-clean reason of the map is covered by the
candidate. Partial means the map loses one reason but keeps others.

| Rank | Candidate | Fully unblocks | Partial |
| ---: | :--- | ---: | ---: |
| none | | | |

A map becomes clean only when every signature that lists it above is fixed.
