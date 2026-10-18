# AGENTS.md — SurrealEngine (fork)

This is a fork of [dpjudas/SurrealEngine](https://github.com/dpjudas/SurrealEngine), a C++20 reimplementation of Unreal Engine 1. Upstream does not accept AI-assisted contributions (see `NO-AI Code Rule.md` at the repository root); this fork never sends work upstream and merges from upstream weekly behind the runtime gate below. Fork remote: https://github.com/tgunnoe/SurrealEngine. Maturity: **molten** (pre-1.0; refactor and cut freely).

## Goal

Unreal (Gold), Unreal Tournament, and Deus Ex run well on this engine. All three are priorities; no order among them. "Runs well" means every claim below holds for each game and is backed by the named evaluator (WIP: T1 holds for all three games under the harness; T2 to T7 are open):

| Claim | Evaluator |
| :--- | :--- |
| T1 Boots to the main menu from the launcher and headless | `Tools/smoke/` harness exit 0 |
| T2 Every shipped map runs 60 s with zero script errors, missing natives, uncaught exceptions, or gameplay stub hits | map-sweep report all green |
| T3 Save → quit → load round-trips player, inventory, flags across three consecutive maps | scripted save/load test |
| T4 Single-player campaign completable (Unreal, Deus Ex); every stock UT map × gametype botmatch finishes with bots scoring | playthrough checklist with per-map harness logs; bot-score assertion |
| T5 No item on [Docs/Status.md](Docs/Status.md)'s known-bug list reproduces | per-item regression scenario |
| T6 ≥ 60 fps at 1920x1080 on the dev GPU on every map | harness fps counter |
| T7 Screenshot parity set passes decorrelated review | screenshot diff + review |

Target versions: Unreal Gold 226b, UT 436, Deus Ex 1112fm (the originals). Out of scope: networking, OldUnreal 227/469, other UE1 games, the editor.

## Requirements
- R1 Evaluators before features. No gameplay fix lands without a harness scenario or unit test that failed first. Grounding: zero project tests existed at fork time; upstream HEAD shipped a crash in Unreal's intro. Signpost: a merged change with no red-then-green evidence in its PR.
- R2 Weekly upstream merge, gated. `Tools/merge-upstream.sh` merges `upstream/master`, builds, runs the smoke suite and native-coverage check; red blocks the merge. Signpost: fork drifts more than two weeks behind upstream, or a merge lands red.
- R3 Every change is reviewable from `git log` and the fork's PRs alone; no process-internal references in commits or PR prose. Signpost: a PR body citing scratch, ledger, or session state.
- R4 The engine never writes inside a game install folder. Config, saves, logs, screenshots, demos and caches go to a per-user directory resolved deterministically from the environment (same install and same environment give the same directory; no install-path hashing, no timestamps), reads fall back to the install, and the engine stays functional when the install is read-only. Grounding: today SE-*.ini files and save games are written into the game's System folder. Signpost: a changed file under a game folder after a run; a run that fails to save or to start because the install is read-only.

## Invariants
- I1 Subtrees `SurrealGPU/`, `SurrealWidgets/`, `SurrealVideo/` and `Thirdparty/` are upstream-owned; the fork does not edit them (work around in engine code; file upstream-subtree bugs as issues). Signpost: a diff touching those paths.
- I3 Version-specific behavior is gated on `LaunchInfo` predicates (`SurrealEngine/GameFolder.h`), never on executable names or hashes elsewhere. Signpost: a new `gameExecutableName ==` comparison outside GameFolder.cpp/UE1GameDatabase.cpp.
- I4 CMakeLists.txt and Configure.js stay in sync when files are added. Signpost: CI builds one and not the other.

## Unknowns
- U1 Which script-declared natives lack C++ handlers per game. Signpost: the native-coverage command's golden output per game.
- U2 Root causes for shock beam, ASMD rings, Nali seed rendering, waving water. Signpost: reproduction scenarios in the sweep.
- U3 Whether per-frame lightmap rebuilds explain the slow UT intro. Signpost: harness fps on CityIntro before and after the lighting change.
- U4 What resets the power-up screen tint in the original engine. Signpost: trace of PlayerPawn flash handling under the harness.
- U5 Deus Ex datacube/book root cause. Signpost: datacube scenario in 00_Training.

## Operational entrypoint
- Read first: [Docs/Status.md](Docs/Status.md) (upstream's bug list), [Docs/Building.md](Docs/Building.md), [Docs/PackageFormat.md](Docs/PackageFormat.md), then `SurrealEngine/Engine.cpp` (`Engine::Run`), `SurrealEngine/VM/Frame.cpp` (`Frame::Call`, `RunExpr`), `SurrealEngine/Package/PackageManager.cpp` (`RegisterFunctions`).
- Build (NixOS): see [Docs/Fork.md](Docs/Fork.md) for the `nix-shell` dependency list; `cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo .. && make -j`.
- Run: `SurrealEngine [--url=<map>] <game folder>`; on wlroots compositors set `SURREALWIDGETS_DISPLAY_BACKEND=X11`. Smoke: `Tools/smoke/` (engine flags `--autostart --exit-after=<s> --smoke-log=<path> --url=<map>`; map sweeps and their triage under `Tools/smoke/scenarios/`). Unit tests: `ctest` in the build dir (target `SurrealTests`). Native coverage: `Tools/native-coverage/check.sh`.
- Game data locations are machine-local; never commit paths.

## Structure
- Core: `SurrealEngine/{VM,Package,Packages,Native,Collision,Render,Light,Audio,Engine.cpp}` — the engine. `Packages/` is the C++ object model per UE1 package; `Native/` binds script natives; `Commandlet/` holds debugger tools including the native extractor.
- Context-sensitive: `Packages/{DeusEx,DeusExText,ConSys,Extension}` are Deus Ex only; `Packages/UPak` Unreal only; `Native/N227*`, `_U227`/`_UT469` handlers are OldUnreal-only and out of scope.
- Not ours: `SurrealGPU/`, `SurrealWidgets/`, `SurrealVideo/`, `Thirdparty/` (I1). `Compiler/` is debugger line-info only, not on the game path.

## Spec pointers
- Terminal claims and campaign plan: this file (Goal) is the authority; the live tracker hydrates from here.
- Process law: predicate `rules.md`, `ambient.md` (installed plugin), not copied here.
