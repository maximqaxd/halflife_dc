@echo off
REM ===========================================================================
REM  set_project_owner.bat
REM ---------------------------------------------------------------------------
REM  A Ghidra non-shared (local) project stores an OWNER in
REM  <project>.rep\project.prp and will NOT open with write access unless that
REM  OWNER matches the CURRENT Windows user. A blank owner does not work either.
REM
REM  After cloning/copying this repo onto a machine, double-click this file once
REM  to stamp every Ghidra project in this folder (*.rep) with YOUR Windows
REM  username so it opens. Safe to re-run; it only rewrites the OWNER value.
REM ===========================================================================
setlocal

echo Current Windows user : %USERNAME%
echo Target folder        : %~dp0
echo.

powershell -NoProfile -ExecutionPolicy Bypass -Command "$q=[char]34; $u=$env:USERNAME; $pat='(NAME='+$q+'OWNER'+$q+'[^>]*VALUE=)'+$q+'[^'+$q+']*'+$q; $rep='${1}'+$q+$u+$q; $n=0; Get-ChildItem -LiteralPath '%~dp0' -Filter *.rep -Directory | ForEach-Object { $f=Join-Path $_.FullName 'project.prp'; if(Test-Path $f){ $t=[IO.File]::ReadAllText($f); $t=[regex]::Replace($t,$pat,$rep); [IO.File]::WriteAllText($f,$t,(New-Object Text.UTF8Encoding($false))); Write-Host ('  OWNER -> '+$u+'    '+$f); $n++ } }; Write-Host ''; Write-Host ('Updated '+$n+' project.prp file(s).')"

echo.
echo Done. You can now open the .gpr in Ghidra.
pause
