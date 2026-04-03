@echo off
setlocal
echo Post-build GDI builder script started.

rem Resolve repo root from the location of this script.
set ROOT=%~dp0
echo ROOT=%ROOT%

rem First argument: path to built EXE (full or relative to repo root).
rem Use %~1 to strip any surrounding quotes that VC6 passes.
set EXE=%~1
echo EXE=%EXE%

if "%EXE%"=="" (
    echo No EXE path passed - post-build may not be configured.
    goto done
)

rem If relative path, resolve against repo root (e.g. obj/WCESH4Dbg/halflife_dc.exe).
set "EXECHECK=%EXE:~0,2%"
if not "%EXECHECK%"==":\" if not "%EXECHECK%"=="\\" if not "%EXE:~0,1%"=="\" (
    set "EXE=%ROOT%%EXE%"
    set "EXE=%EXE:/=\%"
)

if not exist "%EXE%" (
    echo EXE not found: %EXE%
    goto done
)

rem Ensure deploy folder exists.
if not exist "%ROOT%deploy" (
    mkdir "%ROOT%deploy"
    echo Created %ROOT%deploy folder
) else (
    echo deploy folder exists.
)

rem Copy the freshly built EXE into deploy.
echo Copying EXE to deploy folder...
copy /Y "%EXE%" "%ROOT%deploy\"

rem Run buildgdi.exe if present to generate GD-ROM image.
if exist "%ROOT%/utils/buildgdi.exe" (
    echo Building GDI...
    rem ROOT has no spaces, so pass output without extra quotes to avoid path issues.
    "%ROOT%/utils/buildgdi.exe" -data "%ROOT%deploy" -ip "%ROOT%deploy\ip.bin" -output %ROOT%utils -gdi "%ROOT%/utils/Half-Life.GDI" 
    echo Build has finished. 
) else (
    echo buildgdi.exe not found in %ROOT%/utils - skipping GDI step.
)

:done
echo [Post-build] Post-build script finished.
endlocal & exit /b 0

