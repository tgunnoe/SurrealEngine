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

1. Put a `TEST(Name)` block in one of the `*Tests.cpp` files, or add a new file
   and list it in `tests/CMakeLists.txt` (both in the executable sources and in
   the `foreach` that registers tests).
2. Start the file with `#include "Precomp.h"` and `#include "TestRunner.h"`.
3. Use `CHECK`, `CHECK_EQ`, `CHECK_MSG`, and `CHECK_THROWS`. Do not use
   `assert()`: Release builds define `NDEBUG` and it compiles away.
4. Re-run `cmake` so the new name is registered. CMake finds tests by scanning
   for lines that start with `TEST(`, so keep that form.

Temp files go under the system temp directory (`fs::temp_directory_path()`).

## Expected failures

`tests/xfail.txt` lists, one name per line (`#` starts a comment), tests that
are known to fail. For a listed test:

- failing a check is reported as `XFAIL` and the test passes;
- passing is reported as `XPASS` and the test fails, so a fix cannot go
  unnoticed: remove the name from the list in the same change;
- crashing is always a failure.

A listed test appears in `ctest` output as `XFAIL.<name>`. An expected failure
must fail through a check, never through a crash.
