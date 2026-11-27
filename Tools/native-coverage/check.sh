#!/usr/bin/env bash
# Fails when a game has script-declared natives without a C++ handler that the
# committed golden did not already list.
#
# usage: check.sh [--update] <SurrealDebugger> <game folder>...
set -u

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
golden_dir="$here/golden"

update=0
if [ "${1:-}" = "--update" ]; then
	update=1
	shift
fi
if [ $# -lt 2 ]; then
	echo "usage: $0 [--update] <SurrealDebugger> <game folder>..." >&2
	exit 2
fi

debugger="$1"
shift

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

status=0
for folder in "$@"; do
	out="$tmp/out.txt"
	"$debugger" --command "native coverage" --out "$out" "$folder" </dev/null >"$tmp/stdout.txt" 2>"$tmp/stderr.txt"
	rc=$?
	if [ "$rc" -ne 0 ]; then
		echo "FAIL: debugger exited $rc for $folder" >&2
		tr -d '\r' <"$tmp/stdout.txt" >&2
		cat "$tmp/stderr.txt" >&2
		status=1
		continue
	fi

	# Match goldens by the game the debugger reports, not by argument order.
	game="$(tr -d '\r' <"$tmp/stdout.txt" | sed -n 's/^GAME //p' | head -n 1)"
	if [ -z "$game" ] || ! tail -n 1 "$out" | grep -q '^MISSING [0-9][0-9]*$'; then
		echo "FAIL: no GAME name or MISSING line for $folder; debugger stdout:" >&2
		tr -d '\r' <"$tmp/stdout.txt" >&2
		status=1
		continue
	fi
	golden="$golden_dir/$game.txt"
	tr -d '\r' <"$tmp/stdout.txt" | sed -n 's/^SKIPPED \([^:]*\):.*/\1/p' | LC_ALL=C sort -u >"$tmp/skipped.txt"
	skipped_golden="$golden_dir/$game.skipped"

	if [ "$update" = 1 ]; then
		mkdir -p "$golden_dir"
		cp "$out" "$golden"
		if [ -s "$tmp/skipped.txt" ]; then
			cp "$tmp/skipped.txt" "$skipped_golden"
		else
			rm -f "$skipped_golden"
		fi
		echo "updated $golden ($(tail -n 1 "$out"))"
		continue
	fi

	if [ ! -f "$golden" ]; then
		echo "FAIL: no golden for $game (run with --update to create it)" >&2
		status=1
		continue
	fi

	# A package that fails to load hides its natives, so an unknown skip fails
	# the check instead of passing as a smaller set.
	if [ -f "$skipped_golden" ]; then LC_ALL=C sort -u "$skipped_golden" >"$tmp/skipped_old.txt"; else : >"$tmp/skipped_old.txt"; fi
	new_skips="$(LC_ALL=C comm -13 "$tmp/skipped_old.txt" "$tmp/skipped.txt")"
	if [ -n "$new_skips" ]; then
		echo "FAIL: $game skipped packages that failed to load and are not in $game.skipped:" >&2
		echo "$new_skips" >&2
		status=1
		continue
	fi

	grep -v '^MISSING ' "$out" | LC_ALL=C sort -u >"$tmp/new.txt"
	grep -v '^MISSING ' "$golden" | LC_ALL=C sort -u >"$tmp/old.txt"
	added="$(LC_ALL=C comm -13 "$tmp/old.txt" "$tmp/new.txt")"
	if [ -n "$added" ]; then
		echo "FAIL: $game has natives without handlers that the golden does not list:" >&2
		echo "$added" >&2
		status=1
	else
		echo "ok: $game ($(tail -n 1 "$out"), golden $(tail -n 1 "$golden"))"
	fi
done

exit "$status"
