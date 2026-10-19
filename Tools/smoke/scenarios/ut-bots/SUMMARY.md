# UT 436 botmatch sweep summary

Maps: 83. clean: 74 of 83. fail: 0. crash: 1. timeout: 0. pass-verdict but non-clean: 8. warnings: 3337. matches started: 75 of 83.

Check: the verdict counts are recomputed from sweep.json by the `check`
command of the triage module, which exits 0 only if they equal the line
above.

## Warning sites

| Site | Lines | Maps |
| :--- | ---: | ---: |
| Accessed None in UnrealCTFScoreboard.DrawTrailer | 3245 | 1 |
| Accessed None in AssaultRandomizer.SpecialCost | 18 | 1 |
| Accessed None in TMale2Bot.Roaming | 18 | 1 |
| Accessed None in TFemale1Bot.Roaming | 17 | 2 |
| Accessed None in TMale1Bot.Roaming | 11 | 1 |
| Accessed None in TFemale2Bot.Roaming | 9 | 2 |
| Accessed None in UTConsole.AddMessage | 8 | 1 |
| Accessed None in TeamCannon.SeePlayer | 6 | 3 |
| Accessed None in MinigunCannon.SeePlayer | 5 | 1 |

## Failure signatures

| Signature | Maps | Affected |
| :--- | ---: | :--- |
| match did not start | 8 | 8 maps (see TRIAGE.md) |
| crash: FATAL signal 6 | 1 | DM-Tutorial |

## Ranked fix candidates

Fully unblocks means every non-clean reason of the map is covered by the
candidate. Partial means the map loses one reason but keeps others.

| Rank | Candidate | Fully unblocks | Partial |
| ---: | :--- | ---: | ---: |
| 1 | fix signature: match did not start | 8 | 0 |
| 2 | fix signature: crash: FATAL signal 6 | 1 | 0 |

Candidates are ranked by the maps they fully unblock. The
`match did not start` signature is an observation about the run, not a
located defect.
