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
a minus sign allowed, except that a save to a negative slot is refused (slot -1 on Deus Ex
is the quick save and stays allowed). A load of a slot that is not a number or holds no
save is skipped, with a `WARNING` in the log, and the level keeps running; so is a
refused save.

`--exec='5:deletegame <n>'` deletes the save that `savegame <n>` wrote from
`--user-dir` (the `Save<n>` folder on Deus Ex, the `Save<n>` file in the other
games), as the Deus Ex load screen's delete button does, and logs
`Deleted save slot <n>`. Only slots from 1 are deleted, so the quick save is
never touched. A slot that is not a number, is below 1 or holds no save is
skipped, as is every slot under `--autostart` without `--user-dir`, each with a
`WARNING` in the log (`Delete game skipped: ...`).

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

`call <Property>[.<Property>...].<Function> [arg ...]` runs the function on the
object that a path of properties leads to from the player, instead of on the
player or the console. Each hop of the path is `<Name>`, or `<Name>[<index>]`
to take one element of a static array; an index is one to nine decimal digits
counted from 0, with no sign and no spaces, and an array needs one. Every hop
must be an object property of the object before it (a property of a struct or
a dynamic array is not walked), and the text after the last `.` is the
function. The Deus Ex menus are functions of the root window, which the player
keeps in `rootWindow`, so `--exec='10:call rootWindow.InvokeLoadScreen'` opens
the load screen and `call rootWindow.InvokeSaveScreen` the save screen. The
choices of a Deus Ex conversation are buttons in a static array of the choice
window, which the player's conversation object keeps, so once a conversation
has shown its choices a run picks the first with
`--exec='5:call StartConversationByName MeetPaul bind:PaulDenton 0 1;65:call conPlay.conWinThird.conChoices[0].PressButton'`
(the press is timed after the choices appear). No script function picks a
choice by index, and the button's own `PressButton` is the game's way of doing
it: it offers the button to the `ButtonActivated` of its windows, the choice
window's among them, which plays the choice as a mouse click does. The names
are matched without regard to case, the arguments are read as above (`self` is
still the player), and the console is not tried for a path.

A malformed name is refused before any property is read: an empty hop or
function, a bracket out of place, an index that is not digits or has more than
nine, or a bracket in a plain name or in the function. The refusal is written
as `call <name> refused: <reason>`, the name as it was typed, the reason being
one of the above or one of these, where `<path>` is the hops up to and
including the one at fault and `<owner>` the hops that lead to the object that
lacks the property or function (an index is written as a plain number):

| Reason | Cause |
| :--- | :--- |
| `no player` | the run has no player |
| `the player has no property <Name>` | the first hop names no property of the player |
| `the <Class> in <owner> has no property <Name>` | a later hop names no property of the object before it |
| `<path> is not an object property` | the hop's property holds anything but an object |
| `<path> is not an array` | the hop has an index and its property is not a static array |
| `<path> is an array of <N>; give an index` | the hop has no index and its property is a static array |
| `<path> has <N> elements; <index> is out of range` | the index is not below the array's size |
| `<path> is None` | the hop's property, or element, holds no object |
| `the <Class> in <owner> has no function <Function>` | the object the whole path leads to has no such function |

A call that is made logs `call <name> returned <value>`, the name as it was
typed (`call conPlay.conWinThird.conChoices[0].PressButton returned nothing`
for the choice above).

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
object has no name, because `new` gives it none, so in the trace and in the
result of a `call` it shows as `<unnamed ClassName>`, with its class in place of
`ClassName`, for example `<unnamed DeusExGoal>`; `None` means no object at all.

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
| `WARNING <text>` | The harness could not install a signal handler, a requested save was skipped under `--autostart` or because its slot is not a number or is not allowed (`Save game skipped: ...`, for example `slot -2 is not allowed`), a requested load was skipped because its slot is not a number or holds no save (`Load game skipped: ...`), or a requested delete was skipped (`Delete game skipped: ...`). |
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
filename, then the maps of each `--map-subdir` folder), one engine process per
map, each with `--url=<mapname>`. A child still running 45 seconds after its
`--exit-after` is killed with SIGKILL. When `grim` is on PATH, a desktop
screenshot is taken at the midpoint of each run.

Environment (read from the caller, not set by the script):

```
export SURREALWIDGETS_DISPLAY_BACKEND=X11
export LD_LIBRARY_PATH="$(nix-build '<nixpkgs>' -A vulkan-loader --no-out-link)/lib:/run/opengl-driver/lib:$LD_LIBRARY_PATH"
python3 -I Tools/smoke/sweep.py --game "<game folder>" --seconds 15 --limit 3 --out /tmp/report.json
```

Options: `--game`, `--out` (required); `--seconds` (default 15); `--limit`
(first N maps, default all); `--exe` (default `build/SurrealEngine`);
`--logdir` (default `<out>.d`, holds per-map logs and screenshots);
`--url-options`, `--exec`, `--portable`, `--map-subdir`, `--game-by-prefix`
(below).

`--url-options <text>` appends the text to every `--url=<map>`, for example
`?NumBots=4`, so a botmatch run starts `--url=DM-Deck16][?NumBots=4`.
`--exec <text>` passes `--exec=<text>` to every run, for example `5:Fire` to
press fire five seconds after login. `--portable` writes a report that can be
committed: `game` is `<game folder>`, each row's `log` is the log file's name
only, and `screenshot` is `null`. The report records the first two flags as
`url_options` and `exec`, `null` when the flag is absent.

`--map-subdir <name>` also sweeps the maps under `Maps/<name>`, for a game that
keeps maps in a subfolder: Unreal Gold's Return to Na Pali maps are in
`Maps/UPak`, which its `Unreal.ini` lists as a map path. Each runs by its bare
name, as the engine resolves it, so its log is `<mapname>.log` and its URL
`--url=<mapname>`. The top-level maps run first, then each folder's maps in the
order the flags are given; repeat the flag for more folders (a folder named
twice counts once), and `--limit` counts over the whole list. A map name found
in two folders (names that differ only in ASCII case count as one, as they do
to the engine), a folder that does not exist and a name that is not a bare
folder name (empty, `.`, `..`, or holding a path separator) stop the sweep
before any run. The report records the folders as `map_subdirs`, `null` when
the flag is absent.

`--game-by-prefix <PREFIX>=<Package.Class>` names the game class a map runs
under by the map's name prefix, the way the original's menus and servers open
a map with `?game=`: every map whose prefix matches runs as
`--url=<mapname>?game=<Package.Class>`, with the text right after the name and
before `--url-options`. The engine takes the URL's game option before the
map's own `LevelInfo.DefaultGameType` and then the ini's default game, and a
map need not store a game type: none of the stock Unreal Tournament `AS-` maps
does, so without the flag they run as the ini's default deathmatch. The prefix
is the part of the map's name before its first `-`, compared without regard to
ASCII case (a name with no `-` has no prefix and never matches), so
`--game-by-prefix AS=Botpack.Assault` also matches `as-Two`. Repeat the flag
for more prefixes; a prefix that matches no map is unused, and a map that
matches none runs as before. An entry that is not `PREFIX=Package.Class` (a prefix of letters,
digits and underscores; a class of two such words joined by one dot, so it
cannot add a URL option) and a prefix given twice (prefixes that differ only
in ASCII case count as one) stop the sweep before any run. `--url-options` is
not inspected. The report records the entries as `game_by_prefix`, an object
from each prefix as given to its class, `null` when the flag is absent.

Screenshots capture the whole desktop, so a window in front of the engine
appears in them, and an occluded engine window renders slowly.

## Report schema

```
{ "game": "<folder>", "seconds": 15, "url_options": null, "exec": null,
  "game_by_prefix": null, "map_subdirs": null,
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

## Three-map save and load

`saveload.py` checks that a save, quit and load keeps what the player holds
across three consecutive maps. It runs one game's chain as four separate
engine processes on one user directory and writes a JSON report and a markdown
table that say, per hop, which items a load restored, which a travel carried,
which were lost and which were gained.

```
export SURREALWIDGETS_DISPLAY_BACKEND=X11
export LD_LIBRARY_PATH="$(nix-build '<nixpkgs>' -A vulkan-loader --no-out-link)/lib:$(nix-build '<nixpkgs>' -A sdl3.lib --no-out-link)/lib:/run/opengl-driver/lib:$LD_LIBRARY_PATH"
python3 -I Tools/smoke/saveload.py --game "<game folder>" --chain unreal \
    --user-dir /tmp/chain/userdir --config-root /tmp/chain/config \
    --out /tmp/chain/saveload.json --markdown /tmp/chain/SAVELOAD.md --portable
```

`--game`, `--chain` (`unreal`, `ut` or `deusex`), `--user-dir`, `--config-root`
and `--out` are required. `--markdown` writes the table; `--seconds` is the run
time per step (default 60); `--exe` (default `build/SurrealEngine`), `--logdir`
(default `<out>.d`) and `--portable` are the sweep runner's. The driver imports
`parse_log` and `classify` from `sweep.py`.

Two directories are required, so that the driver never touches the caller's
files:

- `--user-dir` holds the engine's game files and the saves (`--autostart` writes
  no save without it). A chain needs a fresh one: the driver refuses a
  directory that already holds files, so a stale slot can never stand in for a
  save that was not made.
- `--config-root` becomes the engine's `XDG_CONFIG_HOME`. The engine writes its
  plaintext log, `SE-Log-LastRun.txt`, there, not under `--user-dir`, and only
  when it exits, so the driver removes it before every step and copies it to
  `<logdir>/<step>.engine.txt` after. The path is made absolute, because the
  engine ignores a relative `XDG_CONFIG_HOME`. The driver copies the host's
  `~/.config/SurrealEngine/Settings.json` into it when it holds none, so the run
  picks the same renderer.

### Chains

Each chain is data in `saveload.py`. A time is seconds after the first login, and
`<s>` below is a step's slot.

| Chain | Maps | State set in the first map | Load | Checks (each reads) |
| :--- | :--- | :--- | :--- | :--- |
| `unreal` | `Vortex2`, `Nyleve`, `Dig` | `summon UnrealShare.Stinger`, then a 1.5 second walk forward | `call ClientTravel ?load=<s> 0 0` | `ShowInventory` |
| `ut` | `DM-Deck16][`, `DM-Turbine`, `DM-Codex`, each with `?NumBots=0` | `Fire` (starts the match), `IAmTheOne` (cheats on, which `summon` needs), `summon Botpack.ShockRifle`, a walk forward | `call ClientTravel ?load=<s> 0 0` | `ShowInventory`; a `Fire` five seconds after every travel, so the new map's player is ready |
| `deusex` | `01_NYC_UNATCOIsland`, `01_NYC_UNATCOHQ`, `02_NYC_BatteryPark` | `call AddGoal T3Goal 1` (primary, so the mission change keeps it), `NoteAdd T3Note`, `call SetBoolFlagFromString T3Flag 1` | `LoadGame <s>` | `ShowInventory`, `call FindGoal T3Goal`, `call GetNote None` |

A travel is `call ClientTravel <map><options> 2 1`, the way a map exit travels
(relative, with items). A Deus Ex flag's value cannot be read from the harness,
so its survival is witnessed through the goal and the note, which travel the
same way. A Deus Ex player holds a key ring and nothing more that the harness
can give, so the key ring is that game's inventory witness.

### Steps

Every step starts the first map, so a load step loads a save made on another
map.

| Step | Does |
| :--- | :--- |
| `save1` | sets the state, checks, `savegame 1` |
| `load1-travel` | loads slot 1 (made on map 1), checks, travels to map 2, checks, `savegame 2` |
| `load2-travel` | loads slot 2 (made on map 2), checks, travels to map 3, checks, `savegame 3` |
| `load3` | loads slot 3 (made on map 3), checks |

Five seconds separate a load or a travel from the next command, and ten seconds
the last check from a save. A step's `--exec` text is in its `exec`
field. The Unreal Tournament travelling steps also run with
`--trace-script=Inventory.TravelPreAccept`, which logs each item the engine
offers to the acceptance of a travel.

The chain stops at the first step with a problem and the report keeps the steps
made; the exit code is 0 only when all four steps are free of problems. A step
has a problem when its verdict (`sweep.py`'s, with the last expected map matched
without case) is not `pass`, when its process left no engine log, when a
scheduled command was refused or never ran (`EXEC_FAILED`, `EXEC_SKIPPED`), when
a load step did not log `Loaded game from slot <s>`, when a check it owes is
missing, or when a save it owes was not made.

### Reading the engine log

The engine log's timestamps are level time, which restarts on a load or a
travel, so a line is placed only by its position in the file. The log is split
into check windows at the lines the engine itself writes: `Loaded game from
slot <n>: <map>`, `Client travel to <url>` and `Saved game to <path>`. The
window before the first of these is the check after the state commands, the
one after a load line is the check after the load, and the one after a travel
line the check after the travel. In a window, each `ShowInventory` line
`Inv: <Package>.<Object>` (UT adds ` state <State>`) is one item, kept as the
package, the instance name and the class, which is the instance name without
its trailing digits. A `call <Function> returned <value>` line is a call
result (`None` for no object, `<unnamed Class>` for a script-made object), and a
`trace:` line of `TravelPreAccept` names an offered item. A `Skipping travel
transfer` line in a travel's window means the travel carried nothing.

Each package the engine loads as a map or a save logs one `Level info:
<object> (the level's first actor) in <package>` line, or `Level info:
<object> (by name; the first actor is <class or none>) in <package>` when the
first actor is not a level info, which names the object the game binds to; the
`Entry` package comes first.

### Report schema

```
{ "game": "<folder>", "chain": "unreal", "seconds": 60,
  "steps": [ { "step", "exit_code", "verdict", "maps_loaded", "loaded_slot", "loaded_map",
               "saved_to",
               "checks": [ { "where", "map", "items": [ { "package", "instance", "class" } ],
                             "calls": [ { "function", "value" } ], "weapon" } ],
               "offered", "travel_skipped", "skip_lines", "summoned",
               "exec_failed", "exec_skipped", "engine_log_missing",
               "exec", "trace", "log", "stdout", "engine_log", "problem" } ],
  "hops": [ { "from", "to", "map_from", "map_to", "by", "kept", "lost", "gained",
              "goal", "note", "offered", "travel_skipped" } ] }
```

`where` is `after-state`, `after-load` or `after-travel`; `map` is the map the
check ran on (the first map for the state check, the map a load line names, the
map of a travel's URL). `loaded_slot`, `loaded_map` and `saved_to` come from the
load and save lines and are `null` when the line is absent; `saved_to` is
relative to `--user-dir`. `calls` keeps only the functions the chain checks.
`offered` lists the items the travel's trace named, once each, and
`travel_skipped` is true when the travel's window holds a skip line;
`skip_lines` lists every such line. `summoned` lists the classes a `summon` logged
(`Summon: fabricated a <Class>` on Unreal Gold, `Summon: Fabricate <Class>` on
Unreal Tournament). `problem` is the reason a step stopped the chain, or `null`.
`log`, `stdout` and `engine_log` are the step's smoke log, captured output and
engine log, as basenames under `--portable`.

A hop joins two consecutive checks that were both made: five hops for a full
chain (state to load, load to travel, travel to load, load to travel, travel to
load). `by` is `load` or `travel`. Items compare by class name without case,
because the package (`Save<n>` after a load on Unreal Gold and Unreal Tournament,
the map's package after a travel, and after a load on Deus Ex) and the instance
number change. `kept` reads as restored for a load and carried for a travel,
`lost` is what the earlier check held and the later one does not, and `gained`
the reverse. `goal` and `note` are `found` or `lost` from the last `FindGoal` and
`GetNote` call of the later check, absent where no such call was made;
`offered` and `travel_skipped` appear on travel hops. A load whose map differs
from the one saved, a skipped travel, or a lost item is named in the markdown's
verdict sentence in place of "survive three maps".

A class listed as kept on an Unreal Tournament travel is a default the game
type gives again, not a carried item: a deathmatch destroys travelled items
(`DeathMatchPlus.AcceptInventory`) and then adds the defaults once the player
is ready. The offered list is what shows whether the engine carried the items
to that rule at all.

The committed reports are `scenarios/<game>/saveload.json` and
`SAVELOAD.md`, each from one `--portable` run of this host.

## Triage module
## Triage module

`Tools/smoke/triage/` is the shared code that turns sweep logs into triage
documents: `extract` (logs to `stubs.json` and `errors.json`), `build`
(sidecars to `TRIAGE.md`, `stubs.md`, `SUMMARY.md`), `check` (documents
against sidecars) and `repro` (re-run maps and compare). Everything that
differs between games lives in a `triage.scenario.Scenario` value: class
hierarchy, bare-owner table, error attribution, prose, document formats,
re-run options (`url_options`, `exec`, and `url_game_by_prefix`, the
`(prefix, class)` pairs a sweep's `--game-by-prefix` entries become, so a
re-run and a repro line carry the sweep's `?game=`) and the checker options.
Functions take the scenario and explicit directories, never a literal or the
working directory.

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
  exits 0 only when there is none. When the scenario sets `warning_register`
  and `register_heading` it also fails for every site `errors.json` records
  that has no row under that heading of `scenarios/WARNINGS.md`; only the site
  is compared, not the register's counts.

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
[`scenarios/WARNINGS.md`](scenarios/WARNINGS.md) is the register of the sites
the sweeps have logged, with a read verdict for each; `check` fails on a site
that `errors.json` records and the register lacks under the scenario's heading,
so a new site is read and entered before a sweep passes.

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

Tests, from the repository root (the second is the sweep unittest, the third the
three-map save and load one):

```
python3 -I -B -m unittest discover -s Tools/smoke/triage -p 'test_*.py'
python3 -I -B -m unittest discover -s Tools/smoke -p test_sweep.py
python3 -I -B -m unittest discover -s Tools/smoke -p test_saveload.py
```

The first includes one identity test per scenario that rebuilds the
committed documents from the committed sidecars and compares them with the
committed ones.
