# Distinct STUB messages, Unreal Gold 226b sweep

Distinct stub lines in the logs: 3. Rows below: 3.

| Stub | Maps | Class | Reason |
| :--- | ---: | :--- | :--- |
| `PathNodeIterator.BuildPath()` | 2 | gameplay | PathNodeIterator descends from Actor |
| `PathNodeIterator.GetFirst()` | 2 | gameplay | PathNodeIterator descends from Actor |
| `PathNodeIterator.GetLast()` | 2 | gameplay | PathNodeIterator descends from Actor |

A stub is gameplay when its owner descends from one of the gameplay roots
listed in scenarios/unreal/scenario.py. An owner missing from that
table is unclassified and counts as gameplay until the table is
extended.
