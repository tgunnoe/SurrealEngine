# Unattended smoke runs

Run SurrealEngine into a map for a fixed time and judge the result from a
log file and an exit code, with nobody at the window.

## Engine flags

All flags are opt-in; without them the engine behaves as before.

| Flag | Effect |
| :--- | :--- |
| `--autostart` | Skip the launcher and the crash-reporter check and use the first game folder given on the command line. A fatal exception exits with code 1 and shows no error window. Config and ini files are not saved, so the game folder is left untouched. |
| `--exit-after=<seconds>` | Leave the main loop after this many seconds since process start. |
| `--smoke-log=<path>` | Write the structured event log described below. Each line is written and flushed immediately. Fatal signals (SIGSEGV, SIGABRT, SIGBUS, SIGFPE, SIGILL) append `FATAL signal <n>` and exit with `128+n`. |
| `--url=<mapname>` | Existing flag; selects the map to load. |

Example:

```
SurrealEngine --autostart --exit-after=20 --smoke-log=/tmp/ut.log "<game folder>"
```

To make a deterministic fatal exception, request a map that does not exist
(`--url=NoSuchMap`): the log ends with `EXCEPTION` and `FATAL` lines and the
exit code is 1.

## Log grammar

One event per line: `KIND` or `KIND <text>`.

| Line | Meaning |
| :--- | :--- |
| `GAME <name> <version>` | First line. |
| `MAP <name>` | Map loaded at startup. |
| `LOGIN` | Player logged in. |
| `SCRIPT_ERROR <text>` | A native raised an error that the VM logged. |
| `STUB <text>` | A distinct unimplemented-function message. |
| `MISSING_NATIVE <text>` | An unknown native function aborted the run. |
| `EXCEPTION <text>` | An exception reached the top level. |
| `FPS <value>` | Frames per second, sampled about once per second. |
| `SUMMARY script_errors=N stub_hits=N missing_natives=N exceptions=N fps_avg=X` | Last line of a clean `--exit-after` run. |
| `FATAL <text>` / `FATAL signal <n>` | Last line of a fatal run; `SUMMARY` is then absent. |

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
              "missing_natives", "exceptions", "fps_avg", "log",
              "screenshot", "verdict" } ] }
```

`fps_avg` and `screenshot` are `null` when there are no samples or no
screenshot. `verdict` is one of:

- `timeout`: killed by the sweep.
- `crash`: the log contains a `FATAL signal` line.
- `pass`: exit code 0 and zero script errors, missing natives and exceptions.
- `fail`: anything else.
