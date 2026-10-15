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
| `--url=<mapname>` | Existing flag; selects the map to load. |

Example:

```
SurrealEngine --autostart --exit-after=20 --smoke-log=/tmp/ut.log "<game folder>"
```

To make a deterministic fatal exception, request a map that does not exist
(`--url=NoSuchMap`): the log ends with `EXCEPTION` and `FATAL` lines and the
exit code is 1.

## Log grammar

One event per line: `KIND` or `KIND <text>`. The harness is POSIX-only; the
signal capture is not built on Windows.

Order: the first three lines are `GAME`, `MAP`, `LOGIN`, except that
`SCRIPT_ERROR`, `STUB` and `WARNING` lines may appear before `MAP` and between
`MAP` and `LOGIN`, because script runs while the map loads. Every later map
transition (restart, next URL, save load, client travel, travel to the entry
level) writes a new `MAP`/`LOGIN` pair. A single-map run has exactly one of
each. The entry-map preload is not logged.

| Line | Meaning |
| :--- | :--- |
| `GAME <name> <version>` | First line. |
| `MAP <name>` | A map was entered. |
| `LOGIN` | The player logged in to that map. |
| `WARNING <text>` | The harness could not install a signal handler. |
| `SCRIPT_ERROR <text>` | A native raised an error that the VM logged. |
| `STUB <text>` | A distinct unimplemented-function message. |
| `MISSING_NATIVE <text>` | An unknown native function. Either it aborted the run (followed by `FATAL`), or the VM caught it inside another native's callback and logged it as a script error, in which case the run continues and it is counted here, not as `SCRIPT_ERROR`. |
| `EXCEPTION <text>` | An exception reached the top level; always followed by `FATAL`. |
| `FPS <value>` | Frames per second, sampled about once per second. |
| `SUMMARY script_errors=N stub_hits=N missing_natives=N exceptions=N fps_avg=X` | Last line of a clean `--exit-after` run. |
| `FATAL <text>` / `FATAL signal <n>` | Last line of a fatal run; `SUMMARY` is then absent. A crash during teardown, after the main loop ended, can still append `FATAL signal <n>` after `SUMMARY`. |

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
              "missing_natives", "exceptions", "fps_avg", "maps_loaded",
              "log", "screenshot", "verdict" } ] }
```

`maps_loaded` lists the `MAP` values in order. The report is rewritten after
every map, so an interrupted sweep keeps the rows finished so far. The
per-map log and screenshot are deleted before each child starts. `fps_avg` and
`screenshot` are `null` when there are no samples or no
screenshot. `verdict` is one of:

- `timeout`: killed by the sweep.
- `crash`: the log contains a `FATAL signal` line.
- `pass`: exit code 0, a `SUMMARY` line, and zero script errors, missing natives and exceptions.
- `fail`: anything else.
