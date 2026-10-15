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
	if ! "$debugger" --command "native coverage" --out "$out" "$folder" </dev/null >"$tmp/stdout.txt" 2>"$tmp/stderr.txt"; then
		echo "FAIL: debugger failed for $folder" >&2
		cat "$tmp/stderr.txt" >&2
		status=1
		continue
	fi

	# Match goldens by the game the debugger reports, not by argument order.
	game="$(tr -d '\r' <"$tmp/stdout.txt" | sed -n 's/^GAME //p' | head -n 1)"
	if [ -z "$game" ] || ! tail -n 1 "$out" | grep -q '^MISSING [0-9][0-9]*$'; then
		echo "FAIL: no GAME name or MISSING line for $folder" >&2
		status=1
		continue
	fi
	golden="$golden_dir/$game.txt"
	tr -d '\r' <"$tmp/stdout.txt" | grep '^SKIPPED ' | sed "s/^/warning: $game: /" >&2

	if [ "$update" = 1 ]; then
		mkdir -p "$golden_dir"
		cp "$out" "$golden"
		echo "updated $golden ($(tail -n 1 "$out"))"
		continue
	fi

	if [ ! -f "$golden" ]; then
		echo "FAIL: no golden for $game (run with --update to create it)" >&2
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
