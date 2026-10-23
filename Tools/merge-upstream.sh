#!/usr/bin/env bash
# Gated upstream merge: merges a ref into a scratch branch in a scratch
# worktree and runs every gate there. Never lands anything.
set -euo pipefail

usage() {
	cat <<'USAGE'
usage: merge-upstream.sh [options]

Fetches upstream, merges --ref into a scratch branch created from --base in a
scratch git worktree, then runs the gates: build, unit tests (ctest), native
coverage (check.sh) on three games, and an unattended intro-map run of each
game. Prints one line per check ("<name>: exit <n>") and a final
"VERDICT: GREEN" or "VERDICT: RED".

Nothing is pushed and nothing is merged into --base or any other long-lived
branch. The worktree (<main checkout>/.scratch/merge/<date>-<time>/tree) and
its branch (merge-gate/<date>-<time>) stay in place for inspection, on GREEN
and RED alike, unless --cleanup is given. Logs are in the sibling logs/ dir.

options:
  --ref <ref>      ref to merge (default: upstream/master)
  --base <ref>     ref the scratch branch starts from (default: the branch
                   the caller's HEAD is on)
  --cleanup        remove the worktree and scratch branch when the run ends
  --unreal <dir>   Unreal game folder      (env UNREAL_DIR)
  --ut <dir>       Unreal Tournament folder (env UT_DIR)
  --deusex <dir>   Deus Ex folder          (env DEUSEX_DIR)
  --help           show this text

Game folders default to this host's GOG installs. They are only read.
Vulkan loader: VULKAN_LOADER_LIB overrides the path from nix-build.
A conflicted merge leaves MERGE_HEAD and conflict markers in the worktree.

exit codes:
  0  GREEN (including: nothing to merge)
  1  RED (a gate failed or the merge conflicted)
  2  usage error
  3  precondition failure (no DISPLAY, missing game folder, missing nix-shell,
     fetch failed, not on a branch and no --base)
USAGE
}

die_usage() { echo "merge-upstream.sh: $1" >&2; echo "try --help" >&2; exit 2; }
die_pre() { echo "merge-upstream.sh: precondition failed: $1" >&2; exit 3; }

REF=upstream/master
BASE=""
CLEANUP=0
UNREAL="${UNREAL_DIR:-/home/tgunnoe/Games/gog/unreal-gold/drive_c/GOG Games/Unreal Gold}"
UT="${UT_DIR:-/home/tgunnoe/Games/gog/unreal-tournament-goty/drive_c/GOG Games/Unreal Tournament GOTY}"
DEUSEX="${DEUSEX_DIR:-/home/tgunnoe/Games/gog/gmdx/drive_c/GOG Games/GMDX}"

while [ $# -gt 0 ]; do
	case "$1" in
	--help|-h) usage; exit 0 ;;
	--cleanup) CLEANUP=1; shift ;;
	--ref|--base|--unreal|--ut|--deusex)
		[ $# -ge 2 ] || die_usage "$1 needs a value"
		case "$1" in
		--ref) REF="$2" ;;
		--base) BASE="$2" ;;
		--unreal) UNREAL="$2" ;;
		--ut) UT="$2" ;;
		--deusex) DEUSEX="$2" ;;
		esac
		shift 2 ;;
	*) die_usage "unknown argument: $1" ;;
	esac
done

# --- preconditions (exit 3) ------------------------------------------------
[ -n "${DISPLAY:-}" ] || die_pre "DISPLAY is not set; the engine needs a display"
command -v nix-shell >/dev/null || die_pre "nix-shell not found"
for d in "$UNREAL" "$UT" "$DEUSEX"; do
	[ -f "$d/System/Engine.u" ] || die_pre "no System/Engine.u in game folder: $d"
done
if [ -z "$BASE" ]; then
	BASE="$(git symbolic-ref --short -q HEAD)" || die_pre "HEAD is detached; pass --base <ref>"
fi
git rev-parse --verify -q "$BASE^{commit}" >/dev/null || die_usage "--base is not a commit: $BASE"

common="$(realpath "$(git rev-parse --git-common-dir)")"
main_checkout="$(dirname "$common")"
git fetch upstream || die_pre "git fetch upstream failed"
git rev-parse --verify -q "$REF^{commit}" >/dev/null || die_usage "--ref is not a commit: $REF"

if git merge-base --is-ancestor "$REF" "$BASE"; then
	echo "note: $REF is already contained in $BASE; nothing to merge"
	echo "VERDICT: GREEN"
	exit 0
fi

stamp="$(date +%Y%m%d-%H%M%S)"
run_dir="$main_checkout/.scratch/merge/$stamp"
tree="$run_dir/tree"
logs="$run_dir/logs"
branch="merge-gate/$stamp"
mkdir -p "$logs"
git worktree add -q -b "$branch" "$tree" "$BASE"
echo "worktree: $tree"
echo "branch:   $branch (from $BASE)"

results=()
red=0
record() { # name rc
	results+=("$1: exit $2")
	if [ "$2" -ne 0 ]; then red=1; fi
}

finish() {
	printf '%s\n' "${results[@]}"
	if [ "$red" -eq 0 ]; then
		echo "VERDICT: GREEN"
	else
		echo "VERDICT: RED"
	fi
	if [ "$CLEANUP" -eq 1 ]; then
		git worktree remove --force "$tree"
		git branch -D "$branch" >/dev/null
		echo "cleaned up $tree and $branch"
	else
		echo "left for inspection: $tree (logs: $logs)"
	fi
	exit "$red"
}

# --- merge -----------------------------------------------------------------
rc=0
git -C "$tree" merge --no-edit -m "chore: merge $REF" "$REF" >"$logs/merge.log" 2>&1 || rc=$?
record merge "$rc"
if [ "$rc" -ne 0 ]; then
	cat "$logs/merge.log"
	conflicts="$(git -C "$tree" diff --name-only --diff-filter=U)"
	if [ -n "$conflicts" ]; then
		echo "conflicting files:"
		echo "  ${conflicts//$'\n'/$'\n'  }"
	fi
	finish
fi

# --- build -----------------------------------------------------------------
build="$tree/build"
mkdir -p "$build"
rc=0
nix-shell -p cmake gcc pkg-config alsa-lib dbus systemdLibs libffi openal SDL2 \
	xorg.libX11 xorg.libXi xorg.libXrandr xorg.libXcursor xorg.libXinerama \
	xorg.libXext xorg.libXfixes xorg.libxcb vulkan-loader wayland waylandpp \
	libxkbcommon libGL libglvnd glib fontconfig \
	--run "cd '$build' && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 SurrealEngine SurrealDebugger SurrealTests" \
	>"$logs/build.log" 2>&1 || rc=$?
record build "$rc"
if [ "$rc" -ne 0 ]; then
	tail -n 30 "$logs/build.log"
	finish
fi

# --- run environment ---------------------------------------------------------
export SURREALWIDGETS_DISPLAY_BACKEND=X11
vk="${VULKAN_LOADER_LIB:-}"
if [ -z "$vk" ]; then
	vk="$(nix-build '<nixpkgs>' -A vulkan-loader --no-out-link)/lib" || die_pre "could not locate the Vulkan loader"
fi
export LD_LIBRARY_PATH="$vk:/run/opengl-driver/lib:${LD_LIBRARY_PATH:-}"

# --- unit tests --------------------------------------------------------------
rc=0
(cd "$build" && ctest --no-tests=error --output-on-failure) >"$logs/ctest.log" 2>&1 || rc=$?
record ctest "$rc"
[ "$rc" -eq 0 ] || tail -n 30 "$logs/ctest.log"

# --- native coverage ---------------------------------------------------------
rc=0
bash "$tree/Tools/native-coverage/check.sh" "$build/SurrealDebugger" "$UNREAL" "$UT" "$DEUSEX" >"$logs/native-coverage.log" 2>&1 || rc=$?
record native-coverage "$rc"
[ "$rc" -eq 0 ] || tail -n 30 "$logs/native-coverage.log"

# --- intro runs ----------------------------------------------------------------
intro() { # name folder
	local name="$1" folder="$2" log="$logs/intro-$1.smoke.log" rc=0
	timeout 75 "$build/SurrealEngine" --autostart --exit-after=30 --smoke-log="$log" "$folder" \
		>"$logs/intro-$name.out" 2>&1 </dev/null || rc=$?
	if [ "$rc" -eq 0 ]; then
		local summaries bad
		summaries="$(grep -c '^SUMMARY' "$log" || true)"
		bad="$(grep -cE '^(MISSING_NATIVE|EXCEPTION|FATAL)' "$log" || true)"
		if [ "$summaries" != 1 ] || [ "$bad" != 0 ]; then
			echo "intro-$name: SUMMARY lines=$summaries, MISSING_NATIVE/EXCEPTION/FATAL lines=$bad (see $log)"
			rc=1
		fi
	elif [ "$rc" -eq 124 ]; then
		echo "intro-$name: timed out"
	fi
	record "intro-$name" "$rc"
}
intro unreal "$UNREAL"
intro ut "$UT"
intro deusex "$DEUSEX"

finish
