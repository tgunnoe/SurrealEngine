# Deus Ex 1112fm sweep summary

Maps: 88. clean: 88. fail: 0. crash: 0. timeout: 0. pass-verdict but non-clean (stub-only): 0. warnings: 4.

Check: the verdict counts are recomputed from sweep.json by the `check`
command of the triage module, which exits 0 only if they equal the line
above.

## Warning sites

| Site | Lines | Maps |
| :--- | ---: | ---: |
| Accessed None in TriggerLight.Trigger | 3 | 1 |
| Accessed None in ShakeTrigger.Trigger | 1 | 1 |

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
