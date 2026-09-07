#!/usr/bin/env bash
# ===========================================================================
#  apply_ghidra_patches.sh
# ---------------------------------------------------------------------------
#  Re-applies the two out-of-repo Ghidra tweaks this project depends on, so a
#  fresh Ghidra install on another machine decompiles + scripts like the
#  original. Run once after installing Ghidra on a new box, then restart Ghidra.
#
#   1. SH-4 FPU single-precision pin -- SuperH4.sinc @define FPSCR_PR / FPSCR_SZ
#      forced to "0:1" so the decompiler stops emitting dual-precision
#      (CONCAT44 / soft-float) noise for every FP instruction. Then recompiles
#      the SLEIGH (.sla). Turns Sys_Frame/Host_UpdateFrameStats etc. from
#      unreadable into ~1:1 C.
#   2. MCP script gate -- launch.properties gets GHIDRA_MCP_ALLOW_SCRIPTS=1
#      (env var for getenv) + VMARGS -D (for getProperty) so run_ghidra_script /
#      ExportSymbols.java work over the bridge. Loopback-only, no auth token.
#
#  Usage:  ./apply_ghidra_patches.sh [ghidra_install_dir]
#  The install dir may also come from $GHIDRA_INSTALL_DIR; otherwise the usual
#  locations (~/ghidra*, /opt/ghidra*, /usr/share/ghidra) are searched.
#  Idempotent -- safe to re-run. RESTART Ghidra afterwards.
# ===========================================================================
set -u

die() { printf 'ERROR: %s\n' "$*" >&2; exit 1; }

case "${1:-}" in
	-h|--help) sed -nE '2,/^# ={10,}$/p' "$0"; exit 0 ;;
esac

GHIDRA="${1:-${GHIDRA_INSTALL_DIR:-}}"

if [ -z "$GHIDRA" ]; then
	for cand in "$HOME"/ghidra_*_PUBLIC "$HOME"/ghidra /opt/ghidra_*_PUBLIC /opt/ghidra \
	            /usr/share/ghidra /usr/local/ghidra_*_PUBLIC /usr/local/ghidra; do
		[ -d "$cand/Ghidra/Processors/SuperH4" ] && GHIDRA="$cand"
	done
fi

[ -n "$GHIDRA" ] || die "no Ghidra install found. Pass one as the first argument
or set GHIDRA_INSTALL_DIR."

GHIDRA="$(cd "$GHIDRA" 2>/dev/null && pwd -P)" || die "not a directory: ${1:-$GHIDRA}"

SINC="$GHIDRA/Ghidra/Processors/SuperH4/data/languages/SuperH4.sinc"
SLASPEC="$GHIDRA/Ghidra/Processors/SuperH4/data/languages/SuperH4_le.slaspec"
LAUNCHPROPS="$GHIDRA/support/launch.properties"
SLEIGH="$GHIDRA/support/sleigh"

echo "Ghidra install : $GHIDRA"
echo

[ -f "$SINC" ] || die "SuperH4.sinc not found at:
  $SINC
Pass the correct Ghidra install dir as the first argument."

echo '[1/3] Pinning FPSCR_PR / FPSCR_SZ to single precision ("0:1")...'
[ -f "$SINC.orig" ] || cp -p "$SINC" "$SINC.orig"
sed -i -E \
	-e 's/(@define[[:space:]]+FPSCR_PR[[:space:]]+)"[^"]*"/\1"0:1"/' \
	-e 's/(@define[[:space:]]+FPSCR_SZ[[:space:]]+)"[^"]*"/\1"0:1"/' \
	"$SINC" || die "could not edit $SINC (check write permission on the install)"
echo "  $SINC"
grep -E '@define[[:space:]]+FPSCR_(PR|SZ)[[:space:]]+"' "$SINC" | sed 's/^/  /'

echo
echo '[2/3] Ensuring MCP script gate in launch.properties...'
if [ ! -f "$LAUNCHPROPS" ]; then
	echo '  (skip: launch.properties not found)'
elif grep -q 'GHIDRA_MCP_ALLOW_SCRIPTS' "$LAUNCHPROPS"; then
	echo '  already present'
else
	cat >> "$LAUNCHPROPS" <<'PROPS'

# Half-Life DC RE: allow run_ghidra_script over the MCP bridge (loopback-only).
ENVVARS_LINUX=GHIDRA_MCP_ALLOW_SCRIPTS=1
VMARGS=-DGHIDRA_MCP_ALLOW_SCRIPTS=1
PROPS
	echo '  added GHIDRA_MCP_ALLOW_SCRIPTS=1 (+ VMARGS -D)'
fi

echo
echo '[3/3] Recompiling SLEIGH (SuperH4_le.slaspec) -- benign warnings are normal...'
[ -x "$SLEIGH" ] || die "sleigh compiler not found or not executable: $SLEIGH"
"$SLEIGH" "$SLASPEC"

echo
echo 'Done. RESTART Ghidra for the decompiler + script-gate changes to take effect.'
