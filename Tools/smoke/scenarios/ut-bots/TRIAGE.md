# UT 436 botmatch sweep triage

Non-clean maps: 8 of 83. A map is clean only when it passes, loaded
the requested map, rendered, logged no error line and no gameplay-class
stub, and its match started: the player's state was `PlayerWalking` when
the fire press ran five seconds after login, or became `PlayerWalking`
afterwards. Every map ran with four bots (`?NumBots=4`) in the game type
the map itself selects, including maps that are not matches. `fps_avg` is
not evidence in this sweep: every run rendered at about one frame per
second on a shared host.

Evidence for a row is its verdict line, `maps_loaded`, failing lines and
`match_started` line (sweep.json, errors.json) and its stub list
(stubs.json).

## CityIntro

- verdict: pass, exit code 0
- maps_loaded: ["CityIntro"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- warnings: 0
- match_started: false
- gameplay stubs: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut-bots/CityIntro.log '--url=CityIntro?NumBots=4' --exec=5:Fire "<game folder>"`
- owner: no cause identified: in the logs of the maps listed, the player's state is `None` from within two seconds of login through the fire press, so the press finds no state to act on; the log cannot say whether the map is not a match or the engine drops the player's state
- Status.md: none applies
- reproduced: yes (second run exit code 0; run1 first line `stubs:`; run2 first line `stubs:`)

## EOL_Assault

- verdict: pass, exit code 0
- maps_loaded: ["EOL_Assault"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- warnings: 0
- match_started: false
- gameplay stubs: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut-bots/EOL_Assault.log '--url=EOL_Assault?NumBots=4' --exec=5:Fire "<game folder>"`
- owner: no cause identified: in the logs of the maps listed, the player's state is `None` from within two seconds of login through the fire press, so the press finds no state to act on; the log cannot say whether the map is not a match or the engine drops the player's state
- Status.md: none applies
- reproduced: yes (second run exit code 0; run1 first line `stubs:`; run2 first line `stubs:`)

## EOL_CTF

- verdict: pass, exit code 0
- maps_loaded: ["EOL_CTF"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- warnings: 0
- match_started: false
- gameplay stubs: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut-bots/EOL_CTF.log '--url=EOL_CTF?NumBots=4' --exec=5:Fire "<game folder>"`
- owner: no cause identified: in the logs of the maps listed, the player's state is `None` from within two seconds of login through the fire press, so the press finds no state to act on; the log cannot say whether the map is not a match or the engine drops the player's state
- Status.md: none applies
- reproduced: yes (second run exit code 0; run1 first line `stubs:`; run2 first line `stubs:`)

## EOL_Challenge

- verdict: pass, exit code 0
- maps_loaded: ["EOL_Challenge"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- warnings: 0
- match_started: false
- gameplay stubs: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut-bots/EOL_Challenge.log '--url=EOL_Challenge?NumBots=4' --exec=5:Fire "<game folder>"`
- owner: no cause identified: in the logs of the maps listed, the player's state is `None` from within two seconds of login through the fire press, so the press finds no state to act on; the log cannot say whether the map is not a match or the engine drops the player's state
- Status.md: none applies
- reproduced: yes (second run exit code 0; run1 first line `stubs:`; run2 first line `stubs:`)

## EOL_Deathmatch

- verdict: pass, exit code 0
- maps_loaded: ["EOL_Deathmatch"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- warnings: 0
- match_started: false
- gameplay stubs: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut-bots/EOL_Deathmatch.log '--url=EOL_Deathmatch?NumBots=4' --exec=5:Fire "<game folder>"`
- owner: no cause identified: in the logs of the maps listed, the player's state is `None` from within two seconds of login through the fire press, so the press finds no state to act on; the log cannot say whether the map is not a match or the engine drops the player's state
- Status.md: none applies
- reproduced: yes (second run exit code 0; run1 first line `stubs:`; run2 first line `stubs:`)

## EOL_Domination

- verdict: pass, exit code 0
- maps_loaded: ["EOL_Domination"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- warnings: 0
- match_started: false
- gameplay stubs: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut-bots/EOL_Domination.log '--url=EOL_Domination?NumBots=4' --exec=5:Fire "<game folder>"`
- owner: no cause identified: in the logs of the maps listed, the player's state is `None` from within two seconds of login through the fire press, so the press finds no state to act on; the log cannot say whether the map is not a match or the engine drops the player's state
- Status.md: none applies
- reproduced: not-run

## EOL_Statues

- verdict: pass, exit code 0
- maps_loaded: ["EOL_Statues"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- warnings: 0
- match_started: false
- gameplay stubs: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut-bots/EOL_Statues.log '--url=EOL_Statues?NumBots=4' --exec=5:Fire "<game folder>"`
- owner: no cause identified: in the logs of the maps listed, the player's state is `None` from within two seconds of login through the fire press, so the press finds no state to act on; the log cannot say whether the map is not a match or the engine drops the player's state
- Status.md: none applies
- reproduced: not-run

## UTCredits

- verdict: pass, exit code 0
- maps_loaded: ["UTCredits"]
- failing lines: none (no SCRIPT_ERROR, MISSING_NATIVE, EXCEPTION or FATAL)
- warnings: 0
- match_started: false
- gameplay stubs: none
- repro: `SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/repro-ut-bots/UTCredits.log '--url=UTCredits?NumBots=4' --exec=5:Fire "<game folder>"`
- owner: no cause identified: in the logs of the maps listed, the player's state is `None` from within two seconds of login through the fire press, so the press finds no state to act on; the log cannot say whether the map is not a match or the engine drops the player's state
- Status.md: none applies
- reproduced: not-run
