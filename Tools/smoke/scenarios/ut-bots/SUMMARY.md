# UT 436 botmatch sweep summary

Maps: 83. clean: 75 of 83. fail: 0. crash: 0. timeout: 0. pass-verdict but non-clean: 8. warnings: 3255. matches started: 75 of 83.

Check: the verdict counts are recomputed from sweep.json by the `check`
command of the triage module, which exits 0 only if they equal the line
above.

## Warning sites

| Site | Lines | Maps |
| :--- | ---: | ---: |
| Accessed None in UnrealCTFScoreboard.DrawTrailer | 3190 | 1 |
| Accessed None in AssaultRandomizer.SpecialCost | 20 | 1 |
| Accessed None in TMale1Bot.Roaming | 18 | 2 |
| Accessed None in TMale2Bot.Roaming | 8 | 2 |
| Accessed None in UTConsole.AddMessage | 8 | 1 |
| Accessed None in MinigunCannon.SeePlayer | 5 | 1 |
| Accessed None in TeamCannon.SeePlayer | 5 | 3 |
| Accessed None in TFemale2Bot.Roaming | 1 | 1 |

## Failure signatures

| Signature | Maps | Affected |
| :--- | ---: | :--- |
| match did not start | 8 | 8 maps (see TRIAGE.md) |

## Ranked fix candidates

Fully unblocks means every non-clean reason of the map is covered by the
candidate. Partial means the map loses one reason but keeps others.

| Rank | Candidate | Fully unblocks | Partial |
| ---: | :--- | ---: | ---: |
| 1 | fix signature: match did not start | 8 | 0 |

Candidates are ranked by the maps they fully unblock. The
`match did not start` signature is an observation about the run, not a
located defect.
