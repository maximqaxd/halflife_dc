@echo off
rem Builds the Half-Life Dreamcast content tools with Visual C++ 6.0.
rem Usage: build_tools.bat [debug]
rem Set VCROOT first to pick a particular compiler.

setlocal
if "%VCROOT%"=="" call :findvc
if "%VCROOT%"=="" (
	echo Could not find a Visual C++ 6.0 installation.
	echo Set VCROOT to the VC98 directory and try again.
	exit /b 1
)

rem the compiler needs the shared IDE directory for MSPDB60.DLL
set PATH=%VCROOT%\bin;%VCROOT%\..\Common\MSDev98\Bin;%PATH%
set INCLUDE=%VCROOT%\include
set LIB=%VCROOT%\lib

cl /nologo /? >nul 2>&1
if errorlevel 1 (
	echo %VCROOT%\bin\cl.exe would not run.
	exit /b 1
)

if /i "%1"=="debug" (
	set CFLAGS=/nologo /MTd /W3 /GX /Zi /Od /D WIN32 /D _DEBUG /D _CONSOLE
) else (
	set CFLAGS=/nologo /MT /W3 /GX /O2 /D WIN32 /D NDEBUG /D _CONSOLE
)

set LIBS=kernel32.lib user32.lib gdi32.lib advapi32.lib
set OUT=%~dp0build
if not exist "%OUT%" mkdir "%OUT%"

echo Using %VCROOT%
call :build studiomdl "studiomdl.c write.c neoanim.c neomesh.c tristrip.c bmpread.c ..\common\pvrtex.c ..\common\cmdlib.c ..\common\lbmlib.c ..\common\mathlib.c ..\common\scriplib.c ..\common\trilib.c"
if errorlevel 1 exit /b 1
call :build qlumpy "qlumpy.c quakegrb.c ..\common\pvrtex.c ..\common\cmdlib.c ..\common\lbmlib.c ..\common\scriplib.c ..\common\wadlib.c"
if errorlevel 1 exit /b 1
call :build makels "makels.cpp"
if errorlevel 1 exit /b 1

echo.
echo Tools are in %OUT%
exit /b 0

:build
echo.
echo === %~1 ===
if not exist "%OUT%\%~1" mkdir "%OUT%\%~1"
pushd "%~dp0%~1"
cl %CFLAGS% /I ..\common /Fo"%OUT%\%~1\\" /Fd"%OUT%\%~1\\" /Fe"%OUT%\%~1.exe" %~2 /link %LIBS%
set RC=%ERRORLEVEL%
popd
exit /b %RC%

:findvc
if exist "%ProgramFiles%\Microsoft Visual Studio\VC98\bin\cl.exe" set VCROOT=%ProgramFiles%\Microsoft Visual Studio\VC98
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\VC98\bin\cl.exe" set VCROOT=%ProgramFiles(x86)%\Microsoft Visual Studio\VC98
if exist "C:\msdev\bin\cl.exe" set VCROOT=C:\msdev
exit /b 0
