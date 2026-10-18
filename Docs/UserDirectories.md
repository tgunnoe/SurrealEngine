# User directories

Surreal Engine never writes inside a game's install folder. Everything it writes
goes to a per-user directory, so a read-only, shared or archived install works as
it is. This page lists where each file goes and how to choose another place.

## Locations

| Platform | Config root | Data root |
| :--- | :--- | :--- |
| Linux and macOS | `$XDG_CONFIG_HOME`, else `~/.config` | `$XDG_DATA_HOME`, else `~/.local/share` |
| Windows | the LocalAppData folder | the LocalAppData folder |

An XDG variable counts only when it is set to an absolute path. When there is no
usable `HOME` either, the engine logs a warning and uses
`<temp folder>/SurrealEngine-<user id>` for both roots. macOS follows the Linux
rule. The Windows and macOS rules are implemented but have not been run.

| What | Where |
| :--- | :--- |
| Launcher settings | `<config root>/SurrealEngine/Settings.json` |
| Crash reports | `<config root>/SurrealEngine/CrashReports/` |
| Log of the last run | `<config root>/SurrealEngine/SE-Log-LastRun.txt` |
| Ini files of one game | `<config root>/SurrealEngine/<key>/SE-<Game>.ini`, `SE-User.ini` and `SE-<name>.ini` for any other ini that was changed |
| Save games of one game | `<data root>/SurrealEngine/<key>/Save/` |
| Files written by the export command | `<data root>/SurrealEngine/<key>/Export/<package>/` |

A directory is created just before the first file goes into it. The `SavePath` set
in a game's own ini is ignored.

## The game key

Each install gets its own directories, named by its key:

```
<executable>-<version>-<folder name>
```

The folder name is the last component of the install path, with every character
outside `A-Z a-z 0-9 . _ -` replaced by `_` and cut to 32 characters. The key
depends on those three values only, so the same install always uses the same
directories, and two installs that differ only by the folder they sit in share
one. A vanilla game and a mod install of the same version live in folders with
different names and so get different keys.

| Install folder | Key |
| :--- | :--- |
| `.../GOG Games/Unreal Gold` (Unreal 226b) | `Unreal-226b-Unreal_Gold` |
| `.../GOG Games/Unreal Tournament GOTY` (Unreal Tournament 436) | `UnrealTournament-436-Unreal_Tournament_GOTY` |
| `.../GOG Games/GMDX` (Deus Ex 1112fm) | `DeusEx-1112fm-GMDX` |

## Reading existing files

An ini is read from the per-user directory first. When it is not there the engine
reads the install's own files: `System/SE-<Game>.ini` from an earlier build, then
`<Game>.ini` and `Default.ini`, and for the user ini `System/SE-User.ini`, then
`User.ini` and `DefUser.ini`; any other ini is looked up as `SE-<name>.ini` in the
per-user directory, then as `<name>.ini` in the install. Those files are only read,
never changed. Saves are
read from the per-user `Save` folder alone.

Nothing is migrated. Ini files and saves that earlier builds wrote into a game
folder stay where they are; copy a save into the new `Save` folder to use it.

## Choosing the directory: --user-dir

`--user-dir=<path>` replaces both the config and the data directory of the game
with `<path>`, so the files above become `<path>/SE-<Game>.ini`,
`<path>/Save/` and `<path>/Export/`. A relative path is taken from the current
directory. The launcher settings, crash reports and last-run log stay under the
config root.

A `--user-dir` or `--smoke-log` inside a game folder named on the command line is
refused: the engine prints `refusing to write inside the game folder: <path>` and
exits with code 2, and the smoke log is checked before it is opened, so an existing
file there is not truncated. Every folder named on the command line is checked, not
only the game that runs. The launcher's saved list of folders is not.

When the game is picked in the launcher, its folder is not known yet when the smoke
log is opened. The log is then created as asked; once the game is chosen, a smoke log
or `--user-dir` inside its folder still ends the run with code 2 before any package
loads, and every later write is guarded as usual.

## Unattended runs

`--autostart` writes no ini file and no save anywhere, so a harness run leaves no
trace besides the smoke log it was asked for and the last-run log. Adding
`--user-dir` lifts that for the directory it names, which lets a scenario save
and reload without touching a real profile:

```
SurrealEngine --autostart --user-dir=/tmp/run1 '--exec=10:savegame 1' <game folder>
```

## The guard

Every file the engine creates, renames or deletes, and every directory it creates
or removes, is checked against the install folder. A path equal to it or below it
is refused with an error, however it is spelled (`..` segments and symbolic links
are resolved first). Comparison is case-sensitive, which is wrong on a
case-insensitive file system.
