# Native coverage

Lists every UnrealScript function declared `native` for which the engine has no
C++ handler, per game, and fails when that set grows.

## Command

```
SurrealDebugger --command "native coverage" [--out <file>] <game folder>
```

`--command=<cmd>` and `--out=<file>` work too. The run needs no stdin: it
requires exactly one folder argument that holds a known game (never the current
directory or the launcher's saved list), runs the one command, and exits. The
status is 1 if the folder is missing or unrecognised, the command fails, or the
command is unknown (including an unknown `native` subcommand). `--out` applies
to `native coverage` only. The
interactive form `native coverage` inside the debugger prints the same lines.
The debugger still initializes widget resources, so run it with a display
backend available (for example `SURREALWIDGETS_DISPLAY_BACKEND=X11`).

Every `UFunction` with `FunctionFlags::Native` in every loaded package is
checked (the traversal of `native extractfuncs`; `Editor` is skipped). A package
that fails to load is skipped, reported as `SKIPPED <package>: <reason>` on
stdout, and does not abort the run.

## Output grammar

```
<Package>.<Class>.<Function> index=<n>     one per missing native
MISSING <total>                            last line
```

Lines are unique and sorted bytewise (`LC_ALL=C`) by package, class, function.
`index=0` marks a name-bound native (matched by function and class name).
With `--out`, the file holds only these lines. Stdout always carries the
debugger banner and log lines, plus `GAME <executable>-<version>` (for example
`GAME Unreal-226b`) and any `SKIPPED` lines. Without `--out`, the lines also go to stdout.

## Check and goldens

```
bash Tools/native-coverage/check.sh <SurrealDebugger> <game folder>...
```

For each folder the script regenerates the output, picks the golden by the
`GAME` name the debugger prints (`golden/<GAME>.txt`, never by argument order),
and compares sorted sets with `comm`. It exits 1 when a native is missing now
but absent from the golden; unchanged or shrunk sets pass. It also exits 1 if a
run fails (its stdout and exit code are printed) or a golden is absent.

A package that fails to load hides its natives, so `golden/<GAME>.skipped`
lists the package names known to skip for that game (absent means none). A
`SKIPPED` package missing from that list makes the script exit 1 and name it.
`--update` rewrites the list along with the golden.

After closing gaps (or deliberately accepting new ones), refresh with the
explicit flag, then review the diff before committing:

```
bash Tools/native-coverage/check.sh --update <SurrealDebugger> <game folder>...
```

Goldens exist for Unreal 226b, Unreal Tournament 436 and Deus Ex 1112fm.

## Drift risk

`NativeFuncExtractor::FindMissing` and `Frame::CallNative` find a handler
through the same function, `NativeFunctions::FindHandler`
(`SurrealEngine/VM/NativeFunc.cpp`), so a native reported missing here is one
a script call would fail to find. What can still drift is the registrations
themselves: a handler registered under the wrong index or class name makes its
native show up as missing, and `check.sh` fails on the added line.

Deus Ex declares index 2054 twice, for `ConEvent.GetSoundLength` and
`Conversation.ClearBindEvents`. It is declared a shared index with
`NativeFunctions::ShareIndex` in `SurrealEngine/Package/PackageManager.cpp`:
script opcodes carry only the index and mean `Conversation.ClearBindEvents`,
and `ConEvent.GetSoundLength` is found by name. Both have handlers, so neither
appears in the Deus Ex golden.
