# Distinct STUB messages, Deus Ex 1112fm sweep

Distinct stub lines in the logs: 13. Rows below: 13.

| Stub | Maps | Class | Reason |
| :--- | ---: | :--- | :--- |
| `Actor.AIEndEvent` | 88 | gameplay | Actor descends from Actor |
| `LevelInfo.InitEventManager` | 88 | gameplay | LevelInfo descends from LevelInfo |
| `Actor.AIClearEventCallback` | 85 | gameplay | Actor descends from Actor |
| `DumpLocation.HasLocationBeenSaved` | 85 | non-gameplay | DumpLocation descends only from DumpLocation > Object |
| `PlayerPawn.ResetKeyboard` | 85 | gameplay | PlayerPawn descends from PlayerPawn |
| `Actor.AISetEventCallback` | 83 | gameplay | Actor descends from Actor |
| `Actor.AISendEvent` | 81 | gameplay | Actor descends from Actor |
| `Actor.PlayBlendAnim` | 75 | gameplay | Actor descends from Actor |
| `Pawn.AIPickRandomDestination_Deus` | 71 | gameplay | Pawn descends from Pawn |
| `Actor.AIStartEvent` | 52 | gameplay | Actor descends from Actor |
| `Pawn.AICanSee` | 17 | gameplay | Pawn descends from Pawn |
| `Pawn.AIDirectionReachable` | 10 | gameplay | Pawn descends from Pawn |
| `Pawn.ReachablePathnodes` | 1 | gameplay | Pawn descends from Pawn |

Classification follows the gameplay-class definition in the run
boundary: a stub is gameplay when its owner descends from Actor,
Pawn, PlayerPawn, LevelInfo or another listed root. DumpLocation
descends only from Object, so it is non-gameplay by that definition
even though gameplay script calls it.
