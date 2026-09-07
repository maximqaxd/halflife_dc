#!/usr/bin/env bash
# ===========================================================================
#  set_project_owner.sh
# ---------------------------------------------------------------------------
#  A Ghidra non-shared (local) project stores an OWNER in
#  <project>.rep/project.prp and will NOT open with write access unless that
#  OWNER matches the CURRENT user. A blank owner does not work either.
#
#  After cloning/copying this repo onto a machine, run this once to stamp every
#  Ghidra project in this folder (*.rep) with YOUR username so it opens. Safe to
#  re-run; it only rewrites the OWNER value.
#
#  Usage:  ./set_project_owner.sh [--clear-locks] [username]
#          --clear-locks  also remove stale .lock files left by a crashed or
#                         killed Ghidra (only do this with Ghidra closed).
# ===========================================================================
set -u

CLEAR_LOCKS=0
USERNAME=""
for arg in "$@"; do
	case "$arg" in
		--clear-locks) CLEAR_LOCKS=1 ;;
		-h|--help) sed -nE '2,/^# ={10,}$/p' "$0"; exit 0 ;;
		*) USERNAME="$arg" ;;
	esac
done
[ -n "$USERNAME" ] || USERNAME="$(id -un)"

HERE="$(cd "$(dirname "$0")" && pwd -P)"

echo "Current user  : $USERNAME"
echo "Target folder : $HERE"
echo

n=0
for rep in "$HERE"/*.rep; do
	prp="$rep/project.prp"
	[ -f "$prp" ] || continue
	sed -i -E "s@(NAME=\"OWNER\"[^>]*VALUE=)\"[^\"]*\"@\1\"$USERNAME\"@" "$prp"
	echo "  OWNER -> $USERNAME    $prp"
	n=$((n + 1))

	if [ "$CLEAR_LOCKS" -eq 1 ]; then
		for lock in "$rep"/.lock "$rep"/.lock~ "$rep"/*.lock; do
			[ -e "$lock" ] || continue
			rm -f "$lock"
			echo "  removed lock              $lock"
		done
	else
		for lock in "$rep"/.lock "$rep"/.lock~ "$rep"/*.lock; do
			[ -e "$lock" ] || continue
			echo "  NOTE: lock file present   $lock"
			echo "        close Ghidra and re-run with --clear-locks to remove it."
		done
	fi
done

echo
echo "Updated $n project.prp file(s)."
echo "You can now open the .gpr in Ghidra."
