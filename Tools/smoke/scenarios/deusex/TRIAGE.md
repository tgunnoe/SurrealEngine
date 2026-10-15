# Deus Ex 1112fm sweep triage

Non-clean maps: 88 of 88. A map is clean only when it passes, loaded
the requested map, rendered, and logged no error line and no
gameplay-class stub. Every map hit at least one gameplay-class stub,
so every map has a section. `fps_avg` is not evidence in this run
(three sweeps shared one display).

Evidence for a stub-only row is its stub list (stubs.json) and its
exit code and SUMMARY-bearing pass verdict (sweep.json).

## 00_Intro

- verdict: pass, exit code 0
- maps_loaded: ["00_Intro"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIStartEvent`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/00_Intro.log --url=00_Intro "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: yes (second run exit code 0; run1 first line `stubs:Actor.AIClearEventCallback,Actor.AIEndEvent,Actor.AISendEvent,Actor.AISetEventCallback,Actor.AIStartEvent,Actor.PlayBlendAnim,LevelInfo.InitEventManager,Pawn.AIPickRandomDestination_Deus,PlayerP`; run2 first line `stubs:Actor.AIClearEventCallback,Actor.AIEndEvent,Actor.AISendEvent,Actor.AISetEventCallback,Actor.AIStartEvent,Actor.PlayBlendAnim,LevelInfo.InitEventManager,Pawn.AIPickRandomDestination_Deus,PlayerP`)

## 00_Training

- verdict: pass, exit code 0
- maps_loaded: ["00_Training"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/00_Training.log --url=00_Training "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: yes (second run exit code 0; run1 first line `stubs:Actor.AIClearEventCallback,Actor.AIEndEvent,Actor.AISendEvent,Actor.AISetEventCallback,Actor.PlayBlendAnim,LevelInfo.InitEventManager,Pawn.AIPickRandomDestination_Deus,PlayerPawn.ResetKeyboard`; run2 first line `stubs:Actor.AIClearEventCallback,Actor.AIEndEvent,Actor.AISendEvent,Actor.AISetEventCallback,Actor.PlayBlendAnim,LevelInfo.InitEventManager,Pawn.AIPickRandomDestination_Deus,PlayerPawn.ResetKeyboard`)

## 00_TrainingCombat

- verdict: pass, exit code 0
- maps_loaded: ["00_TrainingCombat"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIStartEvent`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.AISetEventCallback`, `Actor.PlayBlendAnim`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/00_TrainingCombat.log --url=00_TrainingCombat "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: yes (second run exit code 0; run1 first line `stubs:Actor.AIClearEventCallback,Actor.AIEndEvent,Actor.AISendEvent,Actor.AISetEventCallback,Actor.AIStartEvent,Actor.PlayBlendAnim,LevelInfo.InitEventManager,PlayerPawn.ResetKeyboard`; run2 first line `stubs:Actor.AIClearEventCallback,Actor.AIEndEvent,Actor.AISendEvent,Actor.AISetEventCallback,Actor.AIStartEvent,Actor.PlayBlendAnim,LevelInfo.InitEventManager,PlayerPawn.ResetKeyboard`)

## 00_TrainingFinal

- verdict: pass, exit code 0
- maps_loaded: ["00_TrainingFinal"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIStartEvent`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/00_TrainingFinal.log --url=00_TrainingFinal "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: yes (second run exit code 0; run1 first line `stubs:Actor.AIClearEventCallback,Actor.AIEndEvent,Actor.AISendEvent,Actor.AISetEventCallback,Actor.AIStartEvent,Actor.PlayBlendAnim,LevelInfo.InitEventManager,PlayerPawn.ResetKeyboard`; run2 first line `stubs:Actor.AIClearEventCallback,Actor.AIEndEvent,Actor.AISendEvent,Actor.AISetEventCallback,Actor.AIStartEvent,Actor.PlayBlendAnim,LevelInfo.InitEventManager,PlayerPawn.ResetKeyboard`)

## 01_NYC_UNATCOHQ

- verdict: pass, exit code 0
- maps_loaded: ["01_NYC_UNATCOHQ"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Pawn.AIPickRandomDestination_Deus`, `Pawn.AIDirectionReachable`, `Actor.AISendEvent`, `Actor.AIStartEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/01_NYC_UNATCOHQ.log --url=01_NYC_UNATCOHQ "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: yes (second run exit code 0; run1 first line `stubs:Actor.AIClearEventCallback,Actor.AIEndEvent,Actor.AISendEvent,Actor.AISetEventCallback,Actor.AIStartEvent,Actor.PlayBlendAnim,LevelInfo.InitEventManager,Pawn.AIDirectionReachable,Pawn.AIPickRand`; run2 first line `stubs:Actor.AIClearEventCallback,Actor.AIEndEvent,Actor.AISendEvent,Actor.AISetEventCallback,Actor.AIStartEvent,Actor.PlayBlendAnim,LevelInfo.InitEventManager,Pawn.AIDirectionReachable,Pawn.AIPickRand`)

## 01_NYC_UNATCOIsland

- verdict: pass, exit code 0
- maps_loaded: ["01_NYC_UNATCOIsland"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AISetEventCallback`, `Actor.AIStartEvent`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/01_NYC_UNATCOIsland.log --url=01_NYC_UNATCOIsland "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 02_NYC_Bar

- verdict: pass, exit code 0
- maps_loaded: ["02_NYC_Bar"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/02_NYC_Bar.log --url=02_NYC_Bar "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 02_NYC_BatteryPark

- verdict: pass, exit code 0
- maps_loaded: ["02_NYC_BatteryPark"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AISetEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AIStartEvent`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/02_NYC_BatteryPark.log --url=02_NYC_BatteryPark "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 02_NYC_FreeClinic

- verdict: pass, exit code 0
- maps_loaded: ["02_NYC_FreeClinic"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`, `Actor.AIStartEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/02_NYC_FreeClinic.log --url=02_NYC_FreeClinic "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 02_NYC_Hotel

- verdict: pass, exit code 0
- maps_loaded: ["02_NYC_Hotel"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Actor.AIStartEvent`, `Pawn.AIDirectionReachable`, `Actor.AISendEvent`, `Pawn.AIPickRandomDestination_Deus`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/02_NYC_Hotel.log --url=02_NYC_Hotel "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 02_NYC_Smug

- verdict: pass, exit code 0
- maps_loaded: ["02_NYC_Smug"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/02_NYC_Smug.log --url=02_NYC_Smug "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 02_NYC_Street

- verdict: pass, exit code 0
- maps_loaded: ["02_NYC_Street"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AISetEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AIStartEvent`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/02_NYC_Street.log --url=02_NYC_Street "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 02_NYC_Underground

- verdict: pass, exit code 0
- maps_loaded: ["02_NYC_Underground"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AISetEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`, `Actor.AIStartEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/02_NYC_Underground.log --url=02_NYC_Underground "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 02_NYC_Warehouse

- verdict: pass, exit code 0
- maps_loaded: ["02_NYC_Warehouse"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AISetEventCallback`, `Actor.AIStartEvent`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/02_NYC_Warehouse.log --url=02_NYC_Warehouse "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 03_NYC_747

- verdict: pass, exit code 0
- maps_loaded: ["03_NYC_747"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/03_NYC_747.log --url=03_NYC_747 "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 03_NYC_Airfield

- verdict: pass, exit code 0
- maps_loaded: ["03_NYC_Airfield"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AISetEventCallback`, `Actor.AIStartEvent`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISendEvent`, `Pawn.AIPickRandomDestination_Deus`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/03_NYC_Airfield.log --url=03_NYC_Airfield "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 03_NYC_AirfieldHeliBase

- verdict: pass, exit code 0
- maps_loaded: ["03_NYC_AirfieldHeliBase"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AISetEventCallback`, `Actor.AIStartEvent`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/03_NYC_AirfieldHeliBase.log --url=03_NYC_AirfieldHeliBase "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 03_NYC_BatteryPark

- verdict: pass, exit code 0
- maps_loaded: ["03_NYC_BatteryPark"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/03_NYC_BatteryPark.log --url=03_NYC_BatteryPark "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 03_NYC_BrooklynBridgeStation

- verdict: pass, exit code 0
- maps_loaded: ["03_NYC_BrooklynBridgeStation"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`, `Actor.AIStartEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/03_NYC_BrooklynBridgeStation.log --url=03_NYC_BrooklynBridgeStation "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 03_NYC_Hangar

- verdict: pass, exit code 0
- maps_loaded: ["03_NYC_Hangar"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/03_NYC_Hangar.log --url=03_NYC_Hangar "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 03_NYC_MolePeople

- verdict: pass, exit code 0
- maps_loaded: ["03_NYC_MolePeople"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIStartEvent`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/03_NYC_MolePeople.log --url=03_NYC_MolePeople "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 03_NYC_UNATCOHQ

- verdict: pass, exit code 0
- maps_loaded: ["03_NYC_UNATCOHQ"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Pawn.AIPickRandomDestination_Deus`, `Pawn.AIDirectionReachable`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/03_NYC_UNATCOHQ.log --url=03_NYC_UNATCOHQ "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 03_NYC_UNATCOIsland

- verdict: pass, exit code 0
- maps_loaded: ["03_NYC_UNATCOIsland"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIStartEvent`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Actor.AISendEvent`, `Pawn.AIPickRandomDestination_Deus`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/03_NYC_UNATCOIsland.log --url=03_NYC_UNATCOIsland "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 04_NYC_Bar

- verdict: pass, exit code 0
- maps_loaded: ["04_NYC_Bar"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/04_NYC_Bar.log --url=04_NYC_Bar "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 04_NYC_BatteryPark

- verdict: pass, exit code 0
- maps_loaded: ["04_NYC_BatteryPark"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/04_NYC_BatteryPark.log --url=04_NYC_BatteryPark "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 04_NYC_Hotel

- verdict: pass, exit code 0
- maps_loaded: ["04_NYC_Hotel"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Pawn.AIPickRandomDestination_Deus`, `Pawn.AIDirectionReachable`, `Actor.AISendEvent`, `Actor.AIStartEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/04_NYC_Hotel.log --url=04_NYC_Hotel "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 04_NYC_NSFHQ

- verdict: pass, exit code 0
- maps_loaded: ["04_NYC_NSFHQ"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AISetEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.AIStartEvent`, `Actor.PlayBlendAnim`, `Actor.AISendEvent`, `Pawn.AIPickRandomDestination_Deus`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/04_NYC_NSFHQ.log --url=04_NYC_NSFHQ "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 04_NYC_Smug

- verdict: pass, exit code 0
- maps_loaded: ["04_NYC_Smug"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.AISetEventCallback`, `Actor.PlayBlendAnim`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/04_NYC_Smug.log --url=04_NYC_Smug "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 04_NYC_Street

- verdict: pass, exit code 0
- maps_loaded: ["04_NYC_Street"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/04_NYC_Street.log --url=04_NYC_Street "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 04_NYC_UNATCOHQ

- verdict: pass, exit code 0
- maps_loaded: ["04_NYC_UNATCOHQ"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Pawn.AIPickRandomDestination_Deus`, `Pawn.AIDirectionReachable`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/04_NYC_UNATCOHQ.log --url=04_NYC_UNATCOHQ "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 04_NYC_UNATCOIsland

- verdict: pass, exit code 0
- maps_loaded: ["04_NYC_UNATCOIsland"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIStartEvent`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/04_NYC_UNATCOIsland.log --url=04_NYC_UNATCOIsland "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 04_NYC_Underground

- verdict: pass, exit code 0
- maps_loaded: ["04_NYC_Underground"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AISetEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.AISendEvent`, `Actor.AIStartEvent`, `Pawn.AIPickRandomDestination_Deus`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/04_NYC_Underground.log --url=04_NYC_Underground "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 05_NYC_UNATCOHQ

- verdict: pass, exit code 0
- maps_loaded: ["05_NYC_UNATCOHQ"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`, `Pawn.AICanSee`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/05_NYC_UNATCOHQ.log --url=05_NYC_UNATCOHQ "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 05_NYC_UNATCOIsland

- verdict: pass, exit code 0
- maps_loaded: ["05_NYC_UNATCOIsland"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AISetEventCallback`, `Actor.AIStartEvent`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISendEvent`, `Pawn.AIPickRandomDestination_Deus`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/05_NYC_UNATCOIsland.log --url=05_NYC_UNATCOIsland "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 05_NYC_UNATCOMJ12lab

- verdict: pass, exit code 0
- maps_loaded: ["05_NYC_UNATCOMJ12lab"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AISetEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AIStartEvent`, `Actor.AISendEvent`, `Pawn.AIPickRandomDestination_Deus`, `Pawn.AICanSee`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/05_NYC_UNATCOMJ12lab.log --url=05_NYC_UNATCOMJ12lab "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 06_HongKong_Helibase

- verdict: pass, exit code 0
- maps_loaded: ["06_HongKong_Helibase"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AISetEventCallback`, `Actor.AIStartEvent`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/06_HongKong_Helibase.log --url=06_HongKong_Helibase "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 06_HongKong_MJ12lab

- verdict: pass, exit code 0
- maps_loaded: ["06_HongKong_MJ12lab"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Actor.AIStartEvent`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`, `Pawn.AICanSee`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/06_HongKong_MJ12lab.log --url=06_HongKong_MJ12lab "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 06_HongKong_Storage

- verdict: pass, exit code 0
- maps_loaded: ["06_HongKong_Storage"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Actor.AIStartEvent`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`, `Pawn.AICanSee`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/06_HongKong_Storage.log --url=06_HongKong_Storage "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 06_HongKong_TongBase

- verdict: fail, exit code 1
- maps_loaded: ["06_HongKong_TongBase"]
- failing lines (first 2 distinct, log order):
  - `EXCEPTION GameInfo login failed: Failed to spawn player actor`
  - `FATAL GameInfo login failed: Failed to spawn player actor`
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/06_HongKong_TongBase.log --url=06_HongKong_TongBase "<game folder>"`
- owner: SurrealEngine/Engine.cpp (login path, throws at line 1004) and the DeusEx GameInfo.Login script that spawns the player; the exception text carries no script cause, so the failing spawn step is not yet identified; unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 41 ('Sometimes opening a map crashes the engine with a "Failed to spawn the player actor" error.'); Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: yes (second run exit code 1; run1 first line `EXCEPTION GameInfo login failed: Failed to spawn player actor`; run2 first line `EXCEPTION GameInfo login failed: Failed to spawn player actor`)

## 06_HongKong_VersaLife

- verdict: pass, exit code 0
- maps_loaded: ["06_HongKong_VersaLife"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/06_HongKong_VersaLife.log --url=06_HongKong_VersaLife "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 06_HongKong_WanChai_Canal

- verdict: fail, exit code 1
- maps_loaded: ["06_HongKong_WanChai_Canal"]
- failing lines (first 2 distinct, log order):
  - `EXCEPTION GameInfo login failed: Failed to spawn player actor`
  - `FATAL GameInfo login failed: Failed to spawn player actor`
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AISetEventCallback`, `Actor.AIEndEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/06_HongKong_WanChai_Canal.log --url=06_HongKong_WanChai_Canal "<game folder>"`
- owner: SurrealEngine/Engine.cpp (login path, throws at line 1004) and the DeusEx GameInfo.Login script that spawns the player; the exception text carries no script cause, so the failing spawn step is not yet identified; unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 41 ('Sometimes opening a map crashes the engine with a "Failed to spawn the player actor" error.'); Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: yes (second run exit code 1; run1 first line `EXCEPTION GameInfo login failed: Failed to spawn player actor`; run2 first line `EXCEPTION GameInfo login failed: Failed to spawn player actor`)

## 06_HongKong_WanChai_Garage

- verdict: pass, exit code 0
- maps_loaded: ["06_HongKong_WanChai_Garage"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Actor.AIStartEvent`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/06_HongKong_WanChai_Garage.log --url=06_HongKong_WanChai_Garage "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 06_HongKong_WanChai_Market

- verdict: pass, exit code 0
- maps_loaded: ["06_HongKong_WanChai_Market"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AISetEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AIStartEvent`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/06_HongKong_WanChai_Market.log --url=06_HongKong_WanChai_Market "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 06_HongKong_WanChai_Street

- verdict: pass, exit code 0
- maps_loaded: ["06_HongKong_WanChai_Street"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Actor.AIStartEvent`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`, `Pawn.AICanSee`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/06_HongKong_WanChai_Street.log --url=06_HongKong_WanChai_Street "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 06_HongKong_WanChai_Underworld

- verdict: pass, exit code 0
- maps_loaded: ["06_HongKong_WanChai_Underworld"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Actor.AIStartEvent`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/06_HongKong_WanChai_Underworld.log --url=06_HongKong_WanChai_Underworld "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 08_NYC_Bar

- verdict: pass, exit code 0
- maps_loaded: ["08_NYC_Bar"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/08_NYC_Bar.log --url=08_NYC_Bar "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 08_NYC_FreeClinic

- verdict: pass, exit code 0
- maps_loaded: ["08_NYC_FreeClinic"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/08_NYC_FreeClinic.log --url=08_NYC_FreeClinic "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 08_NYC_Hotel

- verdict: pass, exit code 0
- maps_loaded: ["08_NYC_Hotel"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.AISetEventCallback`, `Actor.PlayBlendAnim`, `Pawn.AIPickRandomDestination_Deus`, `Pawn.AIDirectionReachable`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/08_NYC_Hotel.log --url=08_NYC_Hotel "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 08_NYC_Smug

- verdict: pass, exit code 0
- maps_loaded: ["08_NYC_Smug"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/08_NYC_Smug.log --url=08_NYC_Smug "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 08_NYC_Street

- verdict: fail, exit code 1
- maps_loaded: ["08_NYC_Street"]
- failing lines (first 2 distinct, log order):
  - `EXCEPTION Script execution error:    Iterator statement without an iterator in UNATCOTroop0.GetOvershootDestination    Call stack:    at ScriptedPawn.Seeking.GetOvershootDestination line 10362: iterator(ReachablePathnodes({ name="NavigationPoint", class=Class }, NavPoint, null, distance)) at ScriptedPawn.Seeking line 10463: MoveTarget = GetOvershootDestination(0.500000)  Script call stack: at ScriptedPawn.Seeking.GetOvershootDestination line 10362: iterator(ReachablePathnodes({ name="NavigationPoint", class=Class }, NavPoint, null, distance)) at ScriptedPawn.Seeking line 10463: MoveTarget = GetOvershootDestination(0.500000)`
  - `FATAL Script execution error:    Iterator statement without an iterator in UNATCOTroop0.GetOvershootDestination    Call stack:    at ScriptedPawn.Seeking.GetOvershootDestination line 10362: iterator(ReachablePathnodes({ name="NavigationPoint", class=Class }, NavPoint, null, distance)) at ScriptedPawn.Seeking line 10463: MoveTarget = GetOvershootDestination(0.500000)  Script call stack: at ScriptedPawn.Seeking.GetOvershootDestination line 10362: iterator(ReachablePathnodes({ name="NavigationPoint", class=Class }, NavPoint, null, distance)) at ScriptedPawn.Seeking line 10463: MoveTarget = GetOvershootDestination(0.500000)`
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Actor.AIStartEvent`, `Pawn.AICanSee`, `Actor.AISendEvent`, `Pawn.AIPickRandomDestination_Deus`, `Pawn.ReachablePathnodes`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/08_NYC_Street.log --url=08_NYC_Street "<game folder>"`
- owner: SurrealEngine/Native/NPawn.cpp NPawn::ReachablePathnodes (line 332) is a LogUnimplemented stub that never sets up the iterator, so SurrealEngine/VM/Frame.cpp line 569 throws; this log also holds 'STUB Pawn.ReachablePathnodes'; unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: yes (second run exit code 1; run1 first line `EXCEPTION Script execution error:    Iterator statement without an iterator in UNATCOTroop0.GetOvershootDestination    Call stack:    at ScriptedPawn.Seeking.GetOvershootDestination line 10362: iterat`; run2 first line `EXCEPTION Script execution error:    Iterator statement without an iterator in UNATCOTroop0.GetOvershootDestination    Call stack:    at ScriptedPawn.Seeking.GetOvershootDestination line 10362: iterat`)

## 08_NYC_Underground

- verdict: pass, exit code 0
- maps_loaded: ["08_NYC_Underground"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AISetEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.AISendEvent`, `Actor.PlayBlendAnim`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AIStartEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/08_NYC_Underground.log --url=08_NYC_Underground "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 09_NYC_Dockyard

- verdict: pass, exit code 0
- maps_loaded: ["09_NYC_Dockyard"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AISetEventCallback`, `Actor.AIStartEvent`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/09_NYC_Dockyard.log --url=09_NYC_Dockyard "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 09_NYC_Graveyard

- verdict: pass, exit code 0
- maps_loaded: ["09_NYC_Graveyard"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/09_NYC_Graveyard.log --url=09_NYC_Graveyard "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 09_NYC_Ship

- verdict: pass, exit code 0
- maps_loaded: ["09_NYC_Ship"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/09_NYC_Ship.log --url=09_NYC_Ship "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 09_NYC_ShipBelow

- verdict: crash, exit code -11
- maps_loaded: ["09_NYC_ShipBelow"]
- failing lines (first 1 distinct, log order):
  - `FATAL signal 11`
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AISetEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/09_NYC_ShipBelow.log --url=09_NYC_ShipBelow "<game folder>"`
- owner: unknown native or VM subsystem: SIGSEGV after LOGIN with no script line before it; no debugger was available (gdb not found), so the faulting frame is not identified; unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: none applies; Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: yes (second run exit code -11; run1 first line `FATAL signal 11`; run2 first line `FATAL signal 11`)

## 09_NYC_ShipFan

- verdict: fail, exit code 1
- maps_loaded: ["09_NYC_ShipFan"]
- failing lines (first 2 distinct, log order):
  - `EXCEPTION GameInfo login failed: Failed to spawn player actor`
  - `FATAL GameInfo login failed: Failed to spawn player actor`
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/09_NYC_ShipFan.log --url=09_NYC_ShipFan "<game folder>"`
- owner: SurrealEngine/Engine.cpp (login path, throws at line 1004) and the DeusEx GameInfo.Login script that spawns the player; the exception text carries no script cause, so the failing spawn step is not yet identified; unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 41 ('Sometimes opening a map crashes the engine with a "Failed to spawn the player actor" error.'); Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: yes (second run exit code 1; run1 first line `EXCEPTION GameInfo login failed: Failed to spawn player actor`; run2 first line `EXCEPTION GameInfo login failed: Failed to spawn player actor`)

## 10_Paris_Catacombs

- verdict: pass, exit code 0
- maps_loaded: ["10_Paris_Catacombs"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AISetEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AIStartEvent`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`, `Pawn.AICanSee`, `Pawn.AIDirectionReachable`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/10_Paris_Catacombs.log --url=10_Paris_Catacombs "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 10_Paris_Catacombs_Tunnels

- verdict: pass, exit code 0
- maps_loaded: ["10_Paris_Catacombs_Tunnels"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AISetEventCallback`, `Actor.AIStartEvent`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`, `Pawn.AICanSee`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/10_Paris_Catacombs_Tunnels.log --url=10_Paris_Catacombs_Tunnels "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 10_Paris_Chateau

- verdict: pass, exit code 0
- maps_loaded: ["10_Paris_Chateau"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Pawn.AICanSee`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/10_Paris_Chateau.log --url=10_Paris_Chateau "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 10_Paris_Club

- verdict: pass, exit code 0
- maps_loaded: ["10_Paris_Club"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Actor.AIStartEvent`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/10_Paris_Club.log --url=10_Paris_Club "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 10_Paris_Metro

- verdict: pass, exit code 0
- maps_loaded: ["10_Paris_Metro"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Actor.AIStartEvent`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/10_Paris_Metro.log --url=10_Paris_Metro "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 11_Paris_Cathedral

- verdict: pass, exit code 0
- maps_loaded: ["11_Paris_Cathedral"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AISetEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AIStartEvent`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`, `Pawn.AICanSee`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/11_Paris_Cathedral.log --url=11_Paris_Cathedral "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 11_Paris_Everett

- verdict: pass, exit code 0
- maps_loaded: ["11_Paris_Everett"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`, `Actor.AIStartEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/11_Paris_Everett.log --url=11_Paris_Everett "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 11_Paris_Underground

- verdict: pass, exit code 0
- maps_loaded: ["11_Paris_Underground"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`, `Pawn.AICanSee`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/11_Paris_Underground.log --url=11_Paris_Underground "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 12_Vandenberg_Cmd

- verdict: pass, exit code 0
- maps_loaded: ["12_Vandenberg_Cmd"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AISetEventCallback`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.AIStartEvent`, `Actor.PlayBlendAnim`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`, `Pawn.AICanSee`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/12_Vandenberg_Cmd.log --url=12_Vandenberg_Cmd "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 12_Vandenberg_Computer

- verdict: pass, exit code 0
- maps_loaded: ["12_Vandenberg_Computer"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/12_Vandenberg_Computer.log --url=12_Vandenberg_Computer "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 12_Vandenberg_Gas

- verdict: pass, exit code 0
- maps_loaded: ["12_Vandenberg_Gas"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.AIStartEvent`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/12_Vandenberg_Gas.log --url=12_Vandenberg_Gas "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 12_Vandenberg_Tunnels

- verdict: pass, exit code 0
- maps_loaded: ["12_Vandenberg_Tunnels"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AISetEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`, `Actor.AIStartEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/12_Vandenberg_Tunnels.log --url=12_Vandenberg_Tunnels "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 14_OceanLab_Lab

- verdict: pass, exit code 0
- maps_loaded: ["14_OceanLab_Lab"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AISetEventCallback`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `Actor.AIStartEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`, `Pawn.AICanSee`, `Pawn.AIDirectionReachable`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/14_OceanLab_Lab.log --url=14_OceanLab_Lab "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 14_OceanLab_UC

- verdict: pass, exit code 0
- maps_loaded: ["14_OceanLab_UC"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AISetEventCallback`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`, `Pawn.AICanSee`, `Actor.AIStartEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/14_OceanLab_UC.log --url=14_OceanLab_UC "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 14_Oceanlab_Silo

- verdict: pass, exit code 0
- maps_loaded: ["14_Oceanlab_Silo"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.AISetEventCallback`, `Actor.PlayBlendAnim`, `Actor.AIStartEvent`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/14_Oceanlab_Silo.log --url=14_Oceanlab_Silo "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 14_Vandenberg_Sub

- verdict: pass, exit code 0
- maps_loaded: ["14_Vandenberg_Sub"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AISetEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.AIStartEvent`, `Actor.PlayBlendAnim`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`, `Pawn.AICanSee`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/14_Vandenberg_Sub.log --url=14_Vandenberg_Sub "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 15_Area51_Bunker

- verdict: pass, exit code 0
- maps_loaded: ["15_Area51_Bunker"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AISetEventCallback`, `Actor.AIStartEvent`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/15_Area51_Bunker.log --url=15_Area51_Bunker "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 15_Area51_Entrance

- verdict: pass, exit code 0
- maps_loaded: ["15_Area51_Entrance"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AISetEventCallback`, `Actor.AIStartEvent`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.AISendEvent`, `Actor.PlayBlendAnim`, `Pawn.AIPickRandomDestination_Deus`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/15_Area51_Entrance.log --url=15_Area51_Entrance "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 15_Area51_Final

- verdict: pass, exit code 0
- maps_loaded: ["15_Area51_Final"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.AIStartEvent`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`, `Pawn.AICanSee`, `Pawn.AIDirectionReachable`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/15_Area51_Final.log --url=15_Area51_Final "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 15_Area51_Page

- verdict: pass, exit code 0
- maps_loaded: ["15_Area51_Page"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AISetEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`, `Pawn.AICanSee`, `Pawn.AIDirectionReachable`, `Actor.AIStartEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/15_Area51_Page.log --url=15_Area51_Page "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 99_Endgame1

- verdict: pass, exit code 0
- maps_loaded: ["99_Endgame1"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AISetEventCallback`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AIStartEvent`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/99_Endgame1.log --url=99_Endgame1 "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 99_Endgame2

- verdict: pass, exit code 0
- maps_loaded: ["99_Endgame2"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AISetEventCallback`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AIStartEvent`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/99_Endgame2.log --url=99_Endgame2 "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 99_Endgame3

- verdict: pass, exit code 0
- maps_loaded: ["99_Endgame3"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AISetEventCallback`, `Actor.AIClearEventCallback`, `Actor.AIStartEvent`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/99_Endgame3.log --url=99_Endgame3 "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## 99_Endgame4

- verdict: pass, exit code 0
- maps_loaded: ["99_Endgame4"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.PlayBlendAnim`, `Actor.AISetEventCallback`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/99_Endgame4.log --url=99_Endgame4 "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## DX

- verdict: pass, exit code 0
- maps_loaded: ["DX"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `PlayerPawn.ResetKeyboard`, `Actor.AIEndEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/DX.log --url=DX "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## DXMP_Area51Bunker

- verdict: pass, exit code 0
- maps_loaded: ["DXMP_Area51Bunker"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AISetEventCallback`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Pawn.AIPickRandomDestination_Deus`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/DXMP_Area51Bunker.log --url=DXMP_Area51Bunker "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPawn.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## DXMP_CMD

- verdict: pass, exit code 0
- maps_loaded: ["DXMP_CMD"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AISetEventCallback`, `Actor.AIClearEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/DXMP_CMD.log --url=DXMP_CMD "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## DXMP_Cathedral

- verdict: pass, exit code 0
- maps_loaded: ["DXMP_Cathedral"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AISetEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/DXMP_Cathedral.log --url=DXMP_Cathedral "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## DXMP_Silo

- verdict: pass, exit code 0
- maps_loaded: ["DXMP_Silo"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AISetEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/DXMP_Silo.log --url=DXMP_Silo "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## DXMP_Smuggler

- verdict: pass, exit code 0
- maps_loaded: ["DXMP_Smuggler"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `Actor.AIClearEventCallback`, `Actor.AISetEventCallback`, `Actor.AIEndEvent`, `PlayerPawn.ResetKeyboard`, `Actor.AISendEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/DXMP_Smuggler.log --url=DXMP_Smuggler "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## DXOnly

- verdict: pass, exit code 0
- maps_loaded: ["DXOnly"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `PlayerPawn.ResetKeyboard`, `Actor.AIEndEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/DXOnly.log --url=DXOnly "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run

## Entry

- verdict: pass, exit code 0
- maps_loaded: ["Entry"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- gameplay stubs: `LevelInfo.InitEventManager`, `PlayerPawn.ResetKeyboard`, `Actor.AISendEvent`, `Actor.AIEndEvent`
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-deusex/Entry.log --url=Entry "<game folder>"`
- owner: unimplemented natives, SurrealEngine/Native/NActor.cpp, SurrealEngine/Native/NLevelInfo.cpp, SurrealEngine/Native/NPlayerPawn.cpp: the gameplay stubs above are LogUnimplemented bodies
- Status.md: Docs/Status.md line 86 (Deus Ex: 'Some native functions are still not implemented.')
- reproduced: not-run
