# Unattended smoke runs

Run SurrealEngine into a map for a fixed time and judge the result from a
log file and an exit code, with nobody at the window.

## Engine flags

All flags are opt-in; without them the engine behaves as before.

| Flag | Effect |
| :--- | :--- |
| `--autostart` | Skip the launcher and the crash-reporter check and run exactly the first game folder given on the command line; if that folder is not a UE1 game, or none is given, the run fails with a `FATAL` line and exit code 1 (the launcher's saved list and the current directory are never consulted). A fatal exception exits with code 1 and shows no error window. Ini files and save games are not written, so the game folder is left untouched. |
| `--exit-after=<seconds>` | Leave the main loop after this many seconds since process start. |
| `--smoke-log=<path>` | Write the structured event log described below. Each line is written and flushed immediately. With `--autostart`, fatal signals (SIGSEGV, SIGABRT, SIGBUS, SIGFPE, SIGILL) append `FATAL signal <n>`, then re-raise with the default action so the process dies from that signal (exit status `128+n` in a shell) and still dumps core. Without `--autostart` the crash reporter owns the signal handlers and no `FATAL signal` line is written. |
| `--exec=<seconds>:<command>[;<seconds>:<command>...]` | Run each console command this many seconds after the first `LOGIN`, through the engine's own console. The time is a non-negative decimal; the command is everything after the item's first colon, trimmed, and cannot contain `;`. Items run in time order (equal times keep their written order). The log records `EXEC`, `EXEC_FAILED` or `EXEC_SKIPPED` for each. A malformed item (no colon, empty command, bad time) stops the run before the game starts with a message on stderr and exit code 2. |
| `--url=<mapname>` | Existing flag; selects the map to load. |

Example:

```
SurrealEngine --autostart --exit-after=20 --smoke-log=/tmp/ut.log "<game folder>"
```

To start an Unreal Tournament botmatch, which otherwise waits for the
player's ready signal, press fire five seconds after login:

```
SurrealEngine --autostart --exit-after=60 --smoke-log=/tmp/ut.log '--url=DM-Deck16][?NumBots=4' --exec=5:Fire "<game folder>"
```

Other commands work the same way, for example `--exec='10:open DM-Deck16]['`
to travel to another map ten seconds after login.

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
| `WARNING <text>` | The harness could not install a signal handler, or a requested save was skipped under `--autostart`. |
| `SCRIPT_ERROR <text>` | A native raised an error that the VM logged. |
| `SCRIPT_WARNING <text>` | A script accessed None; the original games log this as a warning and continue. A native whose non-optional out parameter receives a nothing-value is skipped, and the access is reported once, by the statement path. A None in-parameter still reaches the native; if that native type-checks the argument, the same access is also logged as `SCRIPT_ERROR`. |
| `STUB <text>` | A distinct unimplemented-function message. |
| `MISSING_NATIVE <text>` | An unknown native function. Either it aborted the run (followed by `FATAL`), or the VM caught it inside another native's callback and logged it as a script error, in which case the run continues and it is counted here, not as `SCRIPT_ERROR`. |
| `EXCEPTION <text>` | An exception reached the top level; always followed by `FATAL`. |
| `EXEC <seconds> <command>` | An `--exec` command ran; `<seconds>` is the scheduled time, not the time the command ran. Written after `LOGIN`, once per command, in schedule order. |
| `EXEC_FAILED <seconds> <command>` | An `--exec` command ran, but the console found neither a built-in command nor an `exec` function of that name. A script `exec` function that rejects its arguments still counts as `EXEC`. |
| `EXEC_SKIPPED <seconds> <command>` | An `--exec` command never ran because the run ended first (or no login happened). Written just before `SUMMARY`; absent after a fatal run. |
| `FPS <value>` | Frames per second, sampled about once per second. |
| `SUMMARY script_errors=N stub_hits=N missing_natives=N exceptions=N script_warnings=N fps_avg=X` | Last line of a clean `--exit-after` run. |
| `FATAL <text>` / `FATAL signal <n>` | Last line of a fatal run; `SUMMARY` is then absent. A crash during teardown, after the main loop ended, can still append `FATAL signal <n>` after `SUMMARY`. |

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
`--logdir` (default `<out>.d`, holds per-map logs and screenshots).

Screenshots capture the whole desktop, so a window in front of the engine
appears in them, and an occluded engine window renders slowly.

## Report schema

```
{ "game": "<folder>", "seconds": 15,
  "maps": [ { "map", "exit_code", "script_errors", "stub_hits",
              "missing_natives", "exceptions", "script_warnings", "fps_avg", "maps_loaded",
              "log", "screenshot", "verdict" } ] }
```

`maps_loaded` lists the `MAP` values in order. The report is rewritten after
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

`Tools/smoke/triage/` is the shared code that turns sweep logs into triage
documents: `extract` (logs to `stubs.json` and `errors.json`), `build`
(sidecars to `TRIAGE.md`, `stubs.md`, `SUMMARY.md`), `check` (documents
against sidecars) and `repro` (re-run maps and compare). Everything that
differs between games lives in a `triage.scenario.Scenario` value: class
hierarchy, bare-owner table, error attribution, prose, document formats,
re-run options and the checker options. Functions take the scenario and
explicit directories, never a literal or the working directory.

A scenario wraps the module in a few lines: put `Tools/smoke` on
`sys.path`, import the scenario value, and call the module function.
`scenarios/deusex/` is the reference: `scenario.py` holds its literals and
the four scripts keep their original command lines.

Tests, from the repository root (the second is the sweep unittest):

```
python3 -I -B -m unittest discover -s Tools/smoke/triage -p 'test_*.py'
python3 -I -B -m unittest discover -s Tools/smoke -p test_sweep.py
```

The first includes a byte-identity test that regenerates the committed
Deus Ex documents from the committed sidecars.
