@echo off
REM ===========================================================================
REM  apply_ghidra_patches.bat
REM ---------------------------------------------------------------------------
REM  Re-applies the two out-of-repo Ghidra tweaks this project depends on, so a
REM  fresh Ghidra install on another machine decompiles + scripts like the
REM  original. Run once after installing Ghidra on a new PC, then restart Ghidra.
REM
REM   1. SH-4 FPU single-precision pin -- SuperH4.sinc @define FPSCR_PR / FPSCR_SZ
REM      forced to "0:1" so the decompiler stops emitting dual-precision
REM      (CONCAT44 / soft-float) noise for every FP instruction. Then recompiles
REM      the SLEIGH (.sla). Turns Sys_Frame/Host_UpdateFrameStats etc. from
REM      unreadable into ~1:1 C.
REM   2. MCP script gate -- launch.properties gets GHIDRA_MCP_ALLOW_SCRIPTS=1
REM      (env var for getenv) + VMARGS -D (for getProperty) so run_ghidra_script /
REM      ExportSymbols.java work over the bridge. Loopback-only, no auth token.
REM
REM  Usage:  apply_ghidra_patches.bat  [ghidra_install_dir]
REM  Default install dir: C:\dev\ghidra_12.1.2_PUBLIC
REM  Idempotent -- safe to re-run. RESTART Ghidra afterwards.
REM ===========================================================================
setlocal
set "GHIDRA=%~1"
if "%GHIDRA%"=="" set "GHIDRA=C:\dev\ghidra_12.1.2_PUBLIC"

set "SINC=%GHIDRA%\Ghidra\Processors\SuperH4\data\languages\SuperH4.sinc"
set "SLASPEC=%GHIDRA%\Ghidra\Processors\SuperH4\data\languages\SuperH4_le.slaspec"
set "LAUNCHPROPS=%GHIDRA%\support\launch.properties"
set "SLEIGH=%GHIDRA%\support\sleigh.bat"

echo Ghidra install : %GHIDRA%
echo.

if not exist "%SINC%" (
	echo ERROR: SuperH4.sinc not found at:
	echo   %SINC%
	echo Pass the correct Ghidra install dir as the first argument.
	pause
	exit /b 1
)

echo [1/3] Pinning FPSCR_PR / FPSCR_SZ to single precision ("0:1")...
powershell -NoProfile -ExecutionPolicy Bypass -Command "$q=[char]34; $f='%SINC%'; $t=[IO.File]::ReadAllText($f); $t=[regex]::Replace($t,'(@define\s+FPSCR_PR\s+)'+$q+'[^'+$q+']*'+$q,('${1}'+$q+'0:1'+$q)); $t=[regex]::Replace($t,'(@define\s+FPSCR_SZ\s+)'+$q+'[^'+$q+']*'+$q,('${1}'+$q+'0:1'+$q)); [IO.File]::WriteAllText($f,$t,(New-Object Text.UTF8Encoding($false))); Write-Host ('  '+$f); Write-Host ('  FPSCR_PR/SZ pinned to '+$q+'0:1'+$q)"

echo.
echo [2/3] Ensuring MCP script gate in launch.properties...
powershell -NoProfile -ExecutionPolicy Bypass -Command "$f='%LAUNCHPROPS%'; if(-not (Test-Path $f)){ Write-Host '  (skip: launch.properties not found)'; exit } $t=[IO.File]::ReadAllText($f); if($t -match 'GHIDRA_MCP_ALLOW_SCRIPTS'){ Write-Host '  already present' } else { $nl=[Environment]::NewLine; $add=$nl+'# Half-Life DC RE: allow run_ghidra_script over the MCP bridge (loopback-only).'+$nl+'ENVVARS_WINDOWS=GHIDRA_MCP_ALLOW_SCRIPTS=1'+$nl+'VMARGS=-DGHIDRA_MCP_ALLOW_SCRIPTS=1'+$nl; [IO.File]::AppendAllText($f,$add,(New-Object Text.UTF8Encoding($false))); Write-Host '  added GHIDRA_MCP_ALLOW_SCRIPTS=1 (+ VMARGS -D)' }"

echo.
echo [3/3] Recompiling SLEIGH (SuperH4_le.slaspec) -- benign warnings are normal...
call "%SLEIGH%" "%SLASPEC%"

echo.
echo Done. RESTART Ghidra for the decompiler + script-gate changes to take effect.
pause
