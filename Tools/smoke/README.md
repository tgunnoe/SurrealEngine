# Unattended smoke runs

Run SurrealEngine into a map for a fixed time and judge the result from a
log file and an exit code, with nobody at the window.

## Engine flags

All flags are opt-in; without them the engine behaves as before.

| Flag | Effect |
| :--- | :--- |
| `--autostart` | Skip the launcher and the crash-reporter check and run exactly the first game folder given on the command line; if that folder is not a UE1 game, or none is given, the run fails with a `FATAL` line and exit code 1 (the launcher's saved list and the current directory are never consulted). A fatal exception exits with code 1 and shows no error window. Ini files and save games are not written, so the game folder is left untouched. |
| `--exit-after=<seconds>` | Leave the main loop after this many seconds since process start. |
| `--smoke-log=<path>` | Write the structured event log described below. Each line is written and flushed immediately. With `--autostart`, fatal signals (SIGSEGV, SIGABRT, SIGBUS, SIGFPE, SIGILL) append `FATAL signal <n>`, then re-raise with the default action so the process dies from that signal (exit status `128+n` in a shell) and still dumps core. A terminate handler is installed with the signal handlers (see `FATAL terminate` below). Without `--autostart` the crash reporter owns the signal handlers and no `FATAL signal` or `FATAL terminate` line is written. |
| `--exec=<seconds>:<command>[;<seconds>:<command>...]` | Run each console command this many seconds after the first `LOGIN`, through the engine's own console. The time is a non-negative decimal; the command is everything after the item's first colon, trimmed, and cannot contain `;`. Items run in time order (equal times keep their written order). The log records `EXEC`, `EXEC_FAILED` or `EXEC_SKIPPED` for each. A malformed item (no colon, empty command, bad time) stops the run before the game starts with a message on stderr and exit code 2. |
| `--url=<mapname>` | Existing flag; selects the map to load. |
| `--user-dir=<dir>` | Keep the game's ini files, save games and exports under `<dir>` instead of the per-user directories, and write them even under `--autostart`. The directory holds one subdirectory per game, named like the default per-game directories (`<executable>-<version>-<folder name>`, for example `Unreal-226b-Unreal_Gold`), so one directory serves several games and a run of one never reads another's `SE-User.ini`. A directory inside a game folder named on the command line is refused with exit code 2. See `Docs/UserDirectories.md`. |
| `--trace-script=<entry>[,<entry>...]` | Write one engine-log line for every script or native function call made on an object whose class, or any ancestor class, is named. An entry is `<Class>` (every function of such objects) or `<Class>.<Function>` (only that function); entries are trimmed and names are matched without regard to case. Operators (`==`, `+`, `!` and the like) are skipped. A traced call writes `trace: <ObjectName> <ClassName> <StateName>: <Function>(<args>)`, the state being `None` for an object with no state and the object name `<unnamed ClassName>` for an object with no name. A string argument is in double quotes, cut to its first 80 characters, with newlines, quotes and backslashes escaped; a name is in single quotes; a bool is `true` or `false`; a byte, int or float is its number; an object is its name, `<unnamed ClassName>` when it has none (every object a script makes with `new` and no name argument), or `None` for no object; an absent argument is `None`; anything else, such as a vector or a struct, is `?`. Only the entry of a call is traced, never its return value. The hook runs where the VM begins a call, before the check that skips a call the object's state ignores, so a traced call may still have done nothing. After 20000 traced calls the single line `trace: stopped after 20000 calls` is written and nothing more. The smoke log is untouched. The lines go to the engine log, which is written when the engine exits, so a run killed by a signal loses the trace. A malformed value stops the run before the game starts with `SurrealEngine: invalid --trace-script: <reason>` on stderr and exit code 2; the reasons are `empty value`, `empty entry in '<value>'`, `bad entry '<entry>': empty class`, `bad entry '<entry>': empty function` and `bad entry '<entry>': more than one dot`. |

Example:

```
SurrealEngine --autostart --exit-after=20 --smoke-log=/tmp/ut.log "<game folder>"
```

To start an Unreal Tournament botmatch, which otherwise waits for the
player's ready signal, press fire five seconds after login:

```
SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/ut.log '--url=DM-Deck16][?NumBots=4' --exec=5:Fire "<game folder>"
```

With the fire press the log shows `STATE 0.0 PlayerWaiting at (909.3, 1916.3, -634.6)`
and then `STATE 6.0 PlayerWalking`; without it the player stays in
`PlayerWaiting` for the whole run.

Other commands work the same way, for example `--exec='10:open DM-Deck16]['`
to travel to another map ten seconds after login.

`--exec='10:savegame <n>'` saves the game to slot `<n>` and needs `--user-dir`,
because `--autostart` writes no save without it. The engine log names the file as
`Saved game to <path>`, relative to `--user-dir`. On Deus Ex, `savegame 0` takes the
next free slot and `savegame -1` writes the `QuickSave` folder, the one the original
game's quick save uses.

A later run on the same `--user-dir` loads the slot: `--exec='5:QuickLoad'` loads
the slot `savegame 9` wrote (the quick-save slot of Unreal Gold),
`--exec='5:call ClientTravel ?load=<n> 0 0'` loads `savegame <n>` in any game
(Unreal Tournament's `QuickLoad` does nothing in a deathmatch game), and
`--exec='5:LoadGame <n>'` loads `savegame <n>` in Deus Ex. A slot is a whole number,
a minus sign allowed. A load of a slot that is not a number or holds no save is
skipped, with a `WARNING` in the log, and the level keeps running.

Two commands let a run move the player and act without a keyboard:
`hold button <name> <seconds>` keeps a player button down (for example
`--exec='8:hold button bDuck 3'`) and `hold axis <name> <speed> <seconds>` keeps a
player axis at a value (for example `--exec='10:hold axis aForward 300 4'`,
which walks forward for four seconds, so the player's footsteps make noise).
`release` ends every hold at once. A hold acts on the player's properties
below the key bindings and ends at the given time, counted in seconds from the
moment the command ran, or at `release`, whichever comes first. A later `hold`
of the same button or axis replaces the earlier one. The name is an input
button or axis of the player, matched without regard to case: a property the
scripts declare as input, a byte or bool for a button (`bFire`, `bRun`,
`bDuck`), a float for an axis (`aForward`, `aStrafe`, `aTurn` and so on).
Any other property, such as `GroundSpeed` or `Physics`, is refused.

Because a hold sits below the bindings, it sets the button the way a held key
does but never runs the commands bound to that key. In particular
`hold button bFire` does not start an Unreal Tournament botmatch, whose waiting
state leaves only through the `Fire` or `AltFire` console commands.

The seconds of a hold are wall-clock seconds on the same clock as `--exec`, not
game seconds, and a hold acts once per frame, so what it achieves depends on the
frame rate: a four-second hold covers about 240 frames at 60 frames per second
and about 4 at one frame per second. The `FPS` lines of the log show that rate.
A run whose `FPS` falls to about 1 during the hold is therefore not evidence
that the held input had no effect.

`call <Function> [arg ...]` runs a function of the player, or failing that of
the console, whatever its flags, so a run can start game logic that no `exec`
function exposes (a Deus Ex conversation, for example):
`--exec='5:call StartConversationByName MeetPaul bind:PaulDenton 0 1'`.
The function name and the keywords below are matched without regard to case.
Each argument is read as the type its parameter declares, one per parameter
in declaration order (the return value takes none), and the call is made with
no further check of the function's flags:

| Parameter | Argument |
| :--- | :--- |
| byte | an integer from 0 to 255 |
| int, float | numeric text |
| bool | `0`, `1`, `true` or `false` |
| string, name | the word as written |
| object | one of the five object forms below |

The five object forms are `self` (the player), `none`, and three lookups in the
loaded level: `name:<ObjectName>` finds the actor with that object name,
`class:<ClassName>` the first actor of exactly that class, and
`bind:<BindName>` the first actor whose `BindName` property has that value
(Deus Ex only). The values are compared without regard to case. An actor that
is being deleted is skipped by all three lookups and is not offered in the
list of a lookup that fails. An optional
parameter left out takes its default value, and a required one left out is a
refusal.

A call that cannot be made is refused, and nothing runs. The causes are an
argument that does not parse as its parameter's type, a required parameter
with no argument, more arguments than parameters, an out parameter other than
the return value, an argument for a parameter of any other type (a vector, a
struct, an array: such a parameter can only be left out, and then only when it is
optional), an object argument in none of the five forms, and a lookup that finds no
actor. The refusal writes `call <Function> refused: <reason>` to the engine
log, naming the parameter, prefix or function at fault; a lookup that finds
nothing also lists up to 20 distinct values the level has for that form, so a
wrong name can be read off the log. The smoke log gets `EXEC_FAILED`, and the
console's own `Unknown command: ...` line follows the refusal line in the
engine log, because the console reports every command that was not made as
unknown.

A call the VM would skip silently is refused the same way, with
`call <Function> skipped: <reason>`: the player's state ignores the function,
the level has not begun play, or the player is being deleted.

A call that is made logs `call <Function> returned <value>` to the engine log
(`True` or `False`, the number, the text, the object's name, `<unnamed ClassName>`
for an object with no name, or `None` for no object, and `nothing` for a function
that returns no value), and each lookup that succeeds
logs `call: <form> resolved to <Name> at (x, y, z)`. The smoke log shows only
`EXEC`: it does not say what the function did. Whatever the script logs goes to
the engine log too. That is the plaintext `SE-Log-LastRun.txt` in the per-user
configuration directory, written when the engine exits, so a run killed by a
signal leaves none.

`--trace-script` and `call` work together to watch what a started function does:

```
SurrealEngine --autostart --exit-after=40 --smoke-log=/tmp/dx.log --url=01_NYC_UNATCOIsland '--exec=5:call StartConversationByName MeetPaul bind:PaulDenton 0 1' --trace-script=ConPlayBase,DeusExPlayer.StartConversationByName "<game folder>"
```

The call itself is the first traced line, then every function of the conversation
object that it set going. Name the classes narrowly. Window classes, and the
functions the player calls on a running conversation on every frame
(`GetForcePlay`, a window's `DrawWindow`), make hundreds of lines a second, and
the 20000-call cap, which counts them too, is spent within seconds of the call:
the stop line's own time in the log says when. Name the class of an object a
script creates (`DeusExGoal`, for one) to see the calls made on it. Such an
object has no name, because `new` gives it none, so it shows in the trace as
`None`, and a `call` that returns it logs `returned None`, just as it does for
no object at all; the traced calls made on the object are what tell the two
apart.

To make a deterministic fatal exception, request a map that does not exist
(`--url=NoSuchMap`): the log ends with `EXCEPTION` and `FATAL` lines and the
exit code is 1.

## Log grammar

One event per line: `KIND` or `KIND <text>`. The harness is POSIX-only; the
signal capture is not built on Windows.

Extension rules: consumers ignore any unknown KIND, parse `SUMMARY` by its
`key=value` keys rather than by position, and take the game version in `GAME`
as the last token of the line (the game name may contain spaces).

`WARNING` lines from signal-handler setup can precede `GAME`, because the
handlers install before the engine starts; apart from that, the first line
is `GAME`.

Order: the first three lines are `GAME`, `MAP`, `LOGIN`, except that
`SCRIPT_ERROR`, `SCRIPT_WARNING`, `STUB` and `WARNING` lines may appear before `MAP` and between
`MAP` and `LOGIN`, because script runs while the map loads. Every later map
transition (restart, next URL, save load, client travel, travel to the entry
level) writes a new `MAP`/`LOGIN` pair. A single-map run has exactly one of
each. The entry-map preload is not logged.

| Line | Meaning |
| :--- | :--- |
| `GAME <name> <version>` | First line. |
| `MAP <name>` | A map was entered. |
| `LOGIN` | The player logged in to that map. |
| `WARNING <text>` | The harness could not install a signal handler, a requested save was skipped under `--autostart` or because its slot is not a number (`Save game skipped: ...`), or a requested load was skipped because its slot is not a number or holds no save (`Load game skipped: ...`). |
| `SCRIPT_ERROR <text>` | A native raised an error that the VM logged. |
| `SCRIPT_WARNING <text>` | A script accessed None; the original games log this as a warning and continue. A native whose non-optional out parameter receives a nothing-value is skipped, and the access is reported once, by the statement path. A None in-parameter still reaches the native; if that native type-checks the argument, the same access is also logged as `SCRIPT_ERROR`. |
| `STUB <text>` | A distinct unimplemented-function message. |
| `MISSING_NATIVE <text>` | An unknown native function. Either it aborted the run (followed by `FATAL`), or the VM caught it inside another native's callback and logged it as a script error, in which case the run continues and it is counted here, not as `SCRIPT_ERROR`. |
| `EXCEPTION <text>` | An exception reached the top level; always followed by `FATAL`. |
| `EXEC <seconds> <command>` | An `--exec` command ran; `<seconds>` is the scheduled time, not the time the command ran. Written after `LOGIN`, once per command, in schedule order. |
| `EXEC_FAILED <seconds> <command>` | An `--exec` command ran, but the console found neither a built-in command nor an `exec` function of that name, or a `hold` command was refused (a duration that is not positive, a speed that is not a number, or a name that is not an input button or axis of the player; the engine log says which), or a `call` command was refused or skipped (an argument or a lookup that failed, or a call the VM would not make; the engine log says which). A script `exec` function that rejects its arguments still counts as `EXEC`. |
| `EXEC_SKIPPED <seconds> <command>` | An `--exec` command never ran because the run ended first (or no login happened). Written just before `SUMMARY`; absent after a fatal run. |
| `STATE <seconds> <state>` | The player's state name, as seen once per frame after the first `LOGIN`; `None` means the player has no state. `<seconds>` counts from that first `LOGIN` with one decimal and keeps counting across maps. A line is written when the state changes and when a new map's player is first seen, even if its state name is unchanged. The first line for each player carries ` at (x, y, z)`, the player's location with one decimal per coordinate. A state entered and left between two frames is not seen. |
| `FPS <value>` | Frames per second, sampled about once per second. |
| `SUMMARY script_errors=N stub_hits=N missing_natives=N exceptions=N script_warnings=N fps_avg=X` | Last line of a clean `--exit-after` run. |
| `FATAL <text>` / `FATAL terminate: <message>` / `FATAL signal <n>` | Last line of a fatal run; `SUMMARY` is then absent. A crash during teardown, after the main loop ended, can still append `FATAL signal <n>` after `SUMMARY`. Whenever the process reaches `std::terminate` under `--autostart`, `FATAL terminate: <message>` is written just before `FATAL signal 6`; `<message>` is the text of the exception in flight, `unknown exception` for one that is not a standard exception, or `no exception` when none is in flight. When the exception was thrown while others were still unwinding (a double fault, such as a destructor that throws during unwinding), the message ends with how many were unwinding and says it names the later throw, not the one that started the unwind. It is written after `SUMMARY` too, when terminate is reached during teardown. |

`EXEC`, `EXEC_FAILED` and `EXEC_SKIPPED` lines are not counted in `SUMMARY`.
`EXEC` and `EXEC_FAILED` lines follow the first `LOGIN`; a command never runs
before it.

`exceptions` in `SUMMARY` is always 0 today, because every exception ends the
run with `FATAL` instead of `SUMMARY`; it is kept so the line shape is stable.

## Sweep runner

`sweep.py` runs every map under `<game>/Maps` (`.unr` and `.dx`, sorted by
filename), one engine process per map, each with `--url=<mapname>`. A child
still running 45 seconds after its `--exit-after` is killed with SIGKILL. When
`grim` is on PATH, a desktop screenshot is taken at the midpoint of each run.

Environment (read from the caller, not set by the script):

```
export SURREALWIDGETS_DISPLAY_BACKEND=X11
export LD_LIBRARY_PATH="$(nix-build '<nixpkgs>' -A vulkan-loader --no-out-link)/lib:/run/opengl-driver/lib:$LD_LIBRARY_PATH"
python3 -I Tools/smoke/sweep.py --game "<game folder>" --seconds 15 --limit 3 --out /tmp/report.json
```

Options: `--game`, `--out` (required); `--seconds` (default 15); `--limit`
(first N maps, default all); `--exe` (default `build/SurrealEngine`);
`--logdir` (default `<out>.d`, holds per-map logs and screenshots);
`--url-options`, `--exec`, `--portable` (below).

`--url-options <text>` appends the text to every `--url=<map>`, for example
`?NumBots=4`, so a botmatch run starts `--url=DM-Deck16][?NumBots=4`.
`--exec <text>` passes `--exec=<text>` to every run, for example `5:Fire` to
press fire five seconds after login. `--portable` writes a report that can be
committed: `game` is `<game folder>`, each row's `log` is the log file's name
only, and `screenshot` is `null`. The report records the first two flags as
`url_options` and `exec`, `null` when the flag is absent.

Screenshots capture the whole desktop, so a window in front of the engine
appears in them, and an occluded engine window renders slowly.

## Report schema

```
{ "game": "<folder>", "seconds": 15, "url_options": null, "exec": null,
  "maps": [ { "map", "exit_code", "script_errors", "stub_hits",
              "missing_natives", "exceptions", "script_warnings", "fps_avg", "maps_loaded",
              "login", "match_started", "log", "screenshot", "verdict" } ] }
```

`maps_loaded` lists the `MAP` values in order. `login` is true when the log
has a `LOGIN` line. `match_started` is true when the player's state was
`PlayerWalking` when the first `EXEC` line was written, or a later `STATE` line
names `PlayerWalking` (an `EXEC_FAILED` or `EXEC_SKIPPED` line is not the fire
press, and a run with no `EXEC` line has no match); neither field affects the
`verdict`. The report is rewritten after
every map, so an interrupted sweep keeps the rows finished so far. The
per-map log and screenshot are deleted before each child starts. `exit_code` is Python's return code, which is a negative signal number
(for example -11) when the child died from a signal, not the shell's 139. `fps_avg` and
`screenshot` are `null` when there are no samples or no
screenshot. `verdict` is one of:

- `timeout`: killed by the sweep.
- `crash`: the log contains a `FATAL signal` line.
- `idle`: exit code 0 and a `SUMMARY` line, but the requested map (the map file's stem) is not in `maps_loaded`, or `fps_avg` is `null`: the run exited cleanly without reaching the map or rendering.
- `pass`: exit code 0, a `SUMMARY` line, the requested map loaded, a sampled `fps_avg`, and zero script errors, missing natives and exceptions. `script_warnings` does not affect the verdict.
- `fail`: anything else.

## Triage module
## Triage module

`Tools/smoke/triage/` is the shared code that turns sweep logs into triage
documents: `extract` (logs to `stubs.json` and `errors.json`), `build`
(sidecars to `TRIAGE.md`, `stubs.md`, `SUMMARY.md`), `check` (documents
against sidecars) and `repro` (re-run maps and compare). Everything that
differs between games lives in a `triage.scenario.Scenario` value: class
hierarchy, bare-owner table, error attribution, prose, document formats,
re-run options and the checker options. Functions take the scenario and
explicit directories, never a literal or the working directory.

A scenario is a directory under `Tools/smoke/scenarios/` whose `scenario.py`
defines exactly one `Scenario` (two or none is an error), beside the sweep's
`sweep.json` and the files the commands read and write. One entry point runs
every command on any scenario directory, from any working directory:

```
python3 -I -B Tools/smoke/triage/__main__.py <scenario dir> extract <logdir>
python3 -I -B Tools/smoke/triage/__main__.py <scenario dir> repro <exe> <game folder> <logdir> <out.json> <map>...
python3 -I -B Tools/smoke/triage/__main__.py <scenario dir> build <repro.json>
python3 -I -B Tools/smoke/triage/__main__.py <scenario dir> check
```

- `extract` reads the sweep's per-map `.log` files in `<logdir>` and writes
  `stubs.json` and `errors.json` into the scenario directory.
- `repro` re-runs the named maps one at a time and writes the comparison
  with the sweep's logs to `<out.json>`; it needs the run environment above.
- `build` reads `sweep.json`, `stubs.json`, `errors.json` and `<repro.json>`
  and writes the three documents into the scenario directory.
- `check` compares the documents with the sidecars, prints
  `sections=<n> nonclean=<n> problems=<n>` and a `FAIL` line per problem, and
  exits 0 only when there is none.

`<repro.json>` and `<out.json>` are relative to the scenario directory unless
absolute; `<logdir>`, `<exe>` and `<game folder>` are relative to the working
directory. Isolated mode adds no directory to `sys.path`, so the file adds
the one that holds the `triage` package itself; `python3 -B -m triage ...`
from `Tools/smoke` runs the same commands. Before loading `scenario.py` the
entry point puts the scenario directory and `Tools/smoke/scenarios` on
`sys.path`, so a scenario can import a module beside it, as the two UT
scenarios import their class tables from `scenarios/ut436_tables.py`.

Warnings are counted and shown (a `warnings` line in each TRIAGE section
and a total in the SUMMARY counts line, each `not recorded` where the
sweep rows carry no `script_warnings`) but do not make a map non-clean
unless the scenario sets `warnings_fail_clean`, because the original
games log `Accessed None` constantly. `extract` also records, per map,
`warning_sites` in `errors.json`: each distinct `SCRIPT_WARNING` text, with
whitespace collapsed and addresses masked, and the number of lines that
carry it. `SUMMARY.md` lists them in a `## Warning sites` table between the
counts line and the failure signatures: one row per site over all maps, with
its line count and the number of maps it appears in, most lines first and
then by name, so a site nobody has read is visible. A row whose `errors.json`
entry has no `warning_sites` field says nothing about its sites, so when any
row lacks it the table ends with an `unknown (<k> rows without a site
breakdown)` row: those rows' `script_warnings` summed (`not recorded` unless
every one has a count) and `<k>`. `TRIAGE.md` sections keep only the count.

The `## Failure signatures` table has one row per distinct reason a map is
non-clean. A crash (verdict `crash`) has exactly one signature, derived from
its sweep row and spelled the same in every scenario: when the exit code is
`-<n>`, `crash: FATAL signal <n>`, followed by ` (terminate: <text>)` when
the row records a `FATAL terminate: <text>` line (the first such line, its
text address-masked and cut to 80 characters, so aborts with different
causes are different signatures); else `crash: FATAL <text>` from its first
recorded `FATAL` line, else `crash: exit <code>`. `extract` records only
the first five distinct error lines per map, so a terminate line beyond
them is not seen and the row reads as a bare signal. A `FATAL` line in any
other row reads `FATAL: <text>`; a scenario's `signature_rules` never apply
to `FATAL` lines, but its `error_owner` can key on a crash signature. A
table with no rows has one `none` row (for the Warning sites table, that
means every row carries `warning_sites` and none has a site), and every
document ends with exactly one newline.

`scenarios/deusex/` is the reference: `scenario.py` holds its literals.
`scenarios/ut-bots/` is the Unreal Tournament botmatch sweep (every map
launched with `?NumBots=4`, fire pressed five seconds after login): a map is
clean only if it is clean by the rules above and its match started
(`require_match_start`). `scenarios/ut/` is the same game without bots, and
`scenarios/unreal/` is Unreal Gold.

A re-run is compared with the sweep's log by `repro`: the runs agree when
their first failing lines match, or with none in either, when their
gameplay stub sets match; the verdict is `no` when the first run failed (a
failing line, a gameplay stub, or under `require_match_start` a match that
did not start) and the second did not. `triage/matchrule.py` holds the
match-start rule, which `sweep.py` and the comparison both import.

Tests, from the repository root (the second is the sweep unittest):

```
python3 -I -B -m unittest discover -s Tools/smoke/triage -p 'test_*.py'
python3 -I -B -m unittest discover -s Tools/smoke -p test_sweep.py
```

The first includes one identity test per scenario that rebuilds the
committed documents from the committed sidecars and compares them with the
committed ones.
