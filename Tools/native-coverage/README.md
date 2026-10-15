# Native coverage

Lists every UnrealScript function declared `native` for which the engine has no
C++ handler, per game, and fails when that set grows.

## Command

```
SurrealDebugger --command "native coverage" [--out <file>] <game folder>
```

The run needs no stdin: it selects the game from the folder argument, runs the
one command, and exits (status 1 if the command fails or no game is found). The
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
With `--out`, the file holds only these lines; stdout then carries just
`GAME <executable>-<version>` (for example `GAME Unreal-226b`) and any
`SKIPPED` lines. Without `--out`, the lines also go to stdout.

## Check and goldens

```
bash Tools/native-coverage/check.sh <SurrealDebugger> <game folder>...
```

For each folder the script regenerates the output, picks the golden by the
`GAME` name the debugger prints (`golden/<GAME>.txt`, never by argument order),
and compares sorted sets with `comm`. It exits 1 when a native is missing now
but absent from the golden; unchanged or shrunk sets pass. It also exits 1 if a
run fails or a golden is absent. `SKIPPED` packages are echoed as warnings.

After closing gaps (or deliberately accepting new ones), refresh with the
explicit flag, then review the diff before committing:

```
bash Tools/native-coverage/check.sh --update <SurrealDebugger> <game folder>...
```

Goldens exist for Unreal 226b, Unreal Tournament 436 and Deus Ex 1112fm.

## Drift risk

The lookup in `NativeFuncExtractor::FindMissing`
(`SurrealEngine/Commandlet/Native/NativeFuncExtractor.cpp`) is a hand-kept copy
of the handler lookup in `Frame::CallNative` (`SurrealEngine/VM/Frame.cpp`
lines 306-357), including the `GetSoundLength` / 2054 special case from
`NativeFunctions::RegisterHandler`. If `CallNative` changes how it finds a
handler, the two silently disagree. Extracting one shared lookup function used
by both is deferred until `Frame.cpp` is free to edit.

`ConEvent.GetSoundLength` (index 2054) is reported missing by design: its index
collides with another native, registration drops it, and `CallNative` would
dispatch the call to the other function's handler.
