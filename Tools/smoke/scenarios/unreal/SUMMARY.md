# Unreal Gold 226b sweep summary

Maps: 55. clean: 54 of 55. fail: 0. crash: 1. timeout: 0. pass-verdict but non-clean: 0. warnings: 125.

Check: the verdict counts are recomputed from sweep.json by check_triage.py,
which exits 0 only if they equal the line above.

## Failure signatures

| Signature | Maps | Affected |
| :--- | ---: | :--- |
| crash: FATAL signal 6 | 1 | ExtremeLab |
| verdict crash | 1 | ExtremeLab |

## Ranked fix candidates

Fully unblocks means every non-clean reason of the map is covered by the
candidate. Partial means the map loses one reason but keeps others.

| Rank | Candidate | Fully unblocks | Partial |
| ---: | :--- | ---: | ---: |
| 1 | fix signature: crash: FATAL signal 6 | 0 | 1 |
| 2 | fix signature: verdict crash | 0 | 1 |

A map becomes clean only when every signature that lists it above is fixed.
