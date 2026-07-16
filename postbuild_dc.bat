@echo off
setlocal
echo Post-build GDI builder script started.

rem Resolve repo root from the location of this script.
set ROOT=%~dp0
echo ROOT=%ROOT%

rem First argument: path to built EXE. %~f1 resolves it to a full path against the
rem caller's working directory (eVC runs this from the .dsp's projects\ dir, so a
rem relative arg like .\..\obj\WCESH4Rel\halflife_dc.exe lands inside the repo).
if "%~1"=="" (
    echo No EXE path passed - post-build may not be configured.
    goto done
)
set "EXE=%~f1"
echo EXE=%EXE%
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
rem
rem GDISRC is machine-specific (the prototype GDI isn't in the repo). Point it at
rem your local Half-Life.GDI to enable the bootable-disc step; left blank it skips.
rem ---------------------------------------------------------------------------
set "GDISRC=C:\Dev\Dreamcast\HLDC prototypes\HLDC\Half-Life.GDI"
set "STAGE=%ROOT%deploy_stage"
set "OUTDIR=%ROOT%..\gdi_out"

if not exist "%ROOT%utils\buildgdi.exe" (
    echo buildgdi.exe not found in %ROOT%utils - skipping GDI step.
    goto done
)
if "%GDISRC%"=="" (
    echo GDISRC not set - skipping bootable-disc step ^(set it to your local Half-Life.GDI to enable^).
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
