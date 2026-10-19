# Deus Ex 1112fm sweep summary

Maps: 88. clean: 86. fail: 0. crash: 2. timeout: 0. pass-verdict but non-clean (stub-only): 0. warnings: 2084009.

Check: the verdict counts are recomputed from sweep.json by the `check`
command of the triage module, which exits 0 only if they equal the line
above.

## Warning sites

| Site | Lines | Maps |
| :--- | ---: | ---: |
| Accessed None in LaserEmitter.CalcTrace | 1648427 | 27 |
| Accessed None in ElectricityEmitter.CalcTrace | 435578 | 22 |
| Accessed None in TriggerLight.Trigger | 3 | 1 |
| Accessed None in ShakeTrigger.Trigger | 1 | 1 |

## Failure signatures

| Signature | Maps | Affected |
| :--- | ---: | :--- |
| crash: FATAL signal 11 | 2 | 11_Paris_Underground, 12_Vandenberg_Cmd |

## Ranked fix candidates

Fully unblocks means every non-clean reason of the map is covered by the
candidate. Partial means the map loses one reason but keeps others.

| Rank | Candidate | Fully unblocks | Partial |
| ---: | :--- | ---: | ---: |
| 1 | fix signature: crash: FATAL signal 11 | 2 | 0 |

A map becomes clean only when every signature that lists it above is fixed.
