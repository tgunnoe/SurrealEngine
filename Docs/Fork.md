# About this fork

This repository is a fork of [dpjudas/SurrealEngine](https://github.com/dpjudas/SurrealEngine)
that completes Unreal Engine 1 support (script VM, natives, game compatibility)
with the help of LLM tools. Upstream asks that such work is not sent to it
(see `NO-AI Code Rule.md` at the repository root), so none of it is: the work stays in this
fork, and upstream is only ever read from.

## Remotes and branches

| Remote | Repository | Use |
| :--- | :--- | :--- |
| `origin` | `tgunnoe/SurrealEngine` | The fork. `master` here is the fork's mainline. |
| `upstream` | `dpjudas/SurrealEngine` | Read-only. Fetch and merge from it; never push to it. |

`master` tracks the fork. Upstream's `master` is merged into the fork, never the
reverse.

## Building on NixOS

Run from an empty `build/` directory at the repository root:

```
nix-shell -p cmake gcc pkg-config alsa-lib dbus systemdLibs libffi openal SDL2 \
  xorg.libX11 xorg.libXi xorg.libXrandr xorg.libXcursor xorg.libXinerama \
  xorg.libXext xorg.libXfixes xorg.libxcb vulkan-loader wayland waylandpp \
  libxkbcommon libGL libglvnd glib fontconfig \
  --run "cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 SurrealEngine SurrealDebugger SurrealTests"
```

[Building.md](Building.md) covers other distributions.

## Run environment

The engine needs an X11 display backend and the Vulkan loader on the library path:

```
export SURREALWIDGETS_DISPLAY_BACKEND=X11
export LD_LIBRARY_PATH="$(nix-build '<nixpkgs>' -A vulkan-loader --no-out-link)/lib:/run/opengl-driver/lib:$LD_LIBRARY_PATH"
```

## Gates

Three checks decide whether a change is acceptable:

- **Unit tests**: `ctest` in `build/`; see [tests/README.md](../tests/README.md).
- **Native coverage**: `Tools/native-coverage/check.sh` fails when a game gains
  script natives that have no C++ handler; see
  [Tools/native-coverage/README.md](../Tools/native-coverage/README.md).
- **Smoke runs**: the engine runs a game's intro map unattended and the log is
  judged by its content; see [Tools/smoke/README.md](../Tools/smoke/README.md).

## Weekly upstream merge

[Tools/merge-upstream.sh](../Tools/merge-upstream.sh) merges upstream and runs
all three gates without touching your checkout:

```
bash Tools/merge-upstream.sh
```

It fetches `upstream`, creates a scratch branch from the branch you are on
(`--base` overrides this), merges `upstream/master` (`--ref` overrides this) in a
scratch worktree under `.scratch/merge/<date>-<time>/`, then builds, runs
`ctest`, runs the native-coverage check on Unreal, Unreal Tournament and Deus Ex,
and runs each game's intro map for 30 seconds. Each check prints as
`<name>: exit <n>`, followed by `VERDICT: GREEN` or `VERDICT: RED`. Exit codes:
0 GREEN, 1 RED, 2 usage error, 3 precondition failure (no `DISPLAY`, missing game
folder, no `nix-shell`).

Game folders come from `--unreal`, `--ut`, `--deusex`, then the `UNREAL_DIR`,
`UT_DIR`, `DEUSEX_DIR` environment variables, then the author's GOG paths. They
are only read. A full run takes several minutes and opens game windows.

The script never pushes and never merges into `master` or `--base`. A RED result
(a failed gate, or a merge conflict, whose files are listed with the markers left
in the worktree) keeps the worktree for inspection.

A verdict covers exactly one tree: the base as it was at the time of the run plus
the merge commit. Land a GREEN result only with
`git merge --ff-only merge-gate/<stamp>` on the base branch. If that refuses
because the base has moved, re-run the gate rather than merging.

`--cleanup` removes only the current run's worktree and `merge-gate/<stamp>`
branch, at the end of that run; it never touches earlier runs. To remove an
earlier run completely, run `git worktree remove --force <tree>` and
`git branch -D merge-gate/<stamp>`, then delete that run's `logs/` directory
(`git worktree remove` alone leaves the branch and the logs behind). Each kept
run holds a full Release build, hundreds of MB.
