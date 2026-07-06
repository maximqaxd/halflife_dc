@echo off
setlocal
echo Post-build GDI builder script started.

rem Resolve repo root from the location of this script.
set ROOT=%~dp0
echo ROOT=%ROOT%

rem First argument: path to built EXE (full or relative to repo root).
set EXE=%~1
echo EXE=%EXE%
if "%EXE%"=="" (
    echo No EXE path passed - post-build may not be configured.
    goto done
)

rem If relative path, resolve against repo root (e.g. obj/WCESH4Rel/halflife_dc.exe).
set "EXECHECK=%EXE:~0,2%"
if not "%EXECHECK%"==":\" if not "%EXECHECK%"=="\\" if not "%EXE:~0,1%"=="\" (
    set "EXE=%ROOT%%EXE%"
    set "EXE=%EXE:/=\%"
)
if not exist "%EXE%" (
    echo EXE not found: %EXE%
    goto done
)

rem ---------------------------------------------------------------------------
rem The bootable disc is the multi-track prototype GDI at %GDISRC% (game data +
rem CDDA baked into track03/05). We DON'T build a fresh data-only disc - the game
rem assets live only on that prototype. Instead we use buildgdi -rebuild to copy
rem the disc and overlay just the files that changed: the freshly built EXE and
rem the WinCE OS image. Overlay names MUST match the ISO (UPPERCASE, no version)
rem so buildgdi REPLACES the existing entries instead of adding duplicates.
rem ---------------------------------------------------------------------------
set GDISRC=C:\dev\gdi2data\Half Life.GDI
set STAGE=%ROOT%deploy_stage
set OUTDIR=C:\dev\hldc_gdi_out

if not exist "%ROOT%utils\buildgdi.exe" (
    echo buildgdi.exe not found in %ROOT%utils - skipping GDI step.
    goto done
)
if not exist "%GDISRC%" (
    echo Source prototype GDI not found: %GDISRC% - skipping GDI step.
    goto done
)

rem Stage the overlay files with the disc's uppercase names.
if not exist "%STAGE%" mkdir "%STAGE%"
echo Staging fresh HALFLIFE_DC.EXE ...
copy /Y "%EXE%" "%STAGE%\HALFLIFE_DC.EXE" >nul
if exist "%ROOT%deploy\0winceos.bin" (
    echo Staging 0WINCEOS.BIN from deploy ...
    copy /Y "%ROOT%deploy\0winceos.bin" "%STAGE%\0WINCEOS.BIN" >nul
)

rem Output needs ~1.2GB free; clear the previous tracks first.
if not exist "%OUTDIR%" mkdir "%OUTDIR%"
del /Q "%OUTDIR%\*" 2>nul

echo Rebuilding bootable GDI (injecting fresh EXE + 0WINCEOS.BIN into prototype)...
"%ROOT%utils\buildgdi.exe" -rebuild -gdi "%GDISRC%" -data "%STAGE%" -output "%OUTDIR%"
echo Bootable GDI ready: %OUTDIR%\disc.gdi

:done
echo [Post-build] Post-build script finished.
endlocal & exit /b 0
