# Unit tests

`SurrealTests` is a single executable linked against `SurrealCommon`, with a
small hand-rolled runner (no third-party framework). CMake registers every
test as its own `ctest` entry, so one crashing test cannot hide the others.

```
cd build
make SurrealTests
ctest --output-on-failure
```

Run one test directly with `./tests/SurrealTests --test <name> --xfail-file ../tests/xfail.txt`,
or list them all with `--list`.

## Adding a test

1. Add a `.cpp` file under `tests/<subsystem>/` (for example `tests/VM/`), or
   a `TEST(Name)` block in an existing one. CMake picks up every `.cpp` under
   `tests/` on its own; `tests/CMakeLists.txt` needs no edit.
2. Start the file with `#include "Precomp.h"` and `#include "TestRunner.h"`.
3. Use `CHECK`, `CHECK_EQ`, `CHECK_MSG`, and `CHECK_THROWS`. Do not use
   `assert()`: Release builds define `NDEBUG` and it compiles away.
4. Keep each declaration on a line that starts with `TEST(`; CMake finds tests
   by scanning for that form.

CMake re-runs configure by itself when a test file is added or edited, so no
manual `cmake` step is needed.

The `RegistryMatchesSources` entry fails when the number of `TEST(` lines
found in the sources differs from the number of tests the executable
registered.

Temp files go under the system temp directory (`fs::temp_directory_path()`).

## Expected failures

Every `xfail.txt` under `tests/` (one per subsystem directory is fine; they are
merged) lists, one name per line (`#` starts a comment), tests that
are known to fail. For a listed test:

- failing a check is reported as `XFAIL` and the test passes;
- passing is reported as `XPASS` and the test fails, so a fix cannot go
  unnoticed: remove the name from the list in the same change;
- crashing is always a failure.

A name that matches no registered test fails the run. A listed test appears in `ctest` output as `XFAIL.<name>`. An expected failure
must fail through a check, never through a crash.

`ArrayInsertOutOfBoundsReturnsFalse` is a placeholder: probing it for real is
unsafe because `ScriptArray::Insert` writes past its new allocation when the
offset is beyond the end, so the test fails by an explicit message instead of
making the call. Replace it with a real check once the native bounds-checks
the offset.
