# Distinct STUB messages, Deus Ex 1112fm sweep

Distinct stub lines in the logs: 1. Rows below: 1.

| Stub | Maps | Class | Reason |
| :--- | ---: | :--- | :--- |
| `DumpLocation.HasLocationBeenSaved` | 88 | non-gameplay | DumpLocation descends only from DumpLocation > Object |

Classification follows the gameplay-class definition in the run
boundary: a stub is gameplay when its owner descends from Actor,
Pawn, PlayerPawn, LevelInfo or another listed root. DumpLocation
descends only from Object, so it is non-gameplay by that definition
even though gameplay script calls it.
