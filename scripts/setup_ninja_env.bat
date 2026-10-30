@echo off

set "HALLEY_NINJA_ARG="

if /i "%VSCMD_ARG_TGT_ARCH%"=="x64" goto FIND_NINJA

set "vswhere=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "vsdir="
for /f "usebackq delims=" %%i in (`"%vswhere%" -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "vsdir=%%i"
if not defined vsdir goto NO_VS
echo Visual Studio at %vsdir%

REM Stop vcvars from changing the working directory
set "startDir=%CD%"
set "VSCMD_START_DIR=%startDir%"
call "%vsdir%\VC\Auxiliary\Build\vcvars64.bat" || exit /b 1
cd /d "%startDir%"

REM Find system ninja
:FIND_NINJA
where ninja >nul 2>nul && exit /b 0

REM Find halley-deps ninja
for %%i in ("%~dp0..\deps\bin\ninja.exe") do set "shippedNinja=%%~fi"
if not exist "%shippedNinja%" goto NO_NINJA
set HALLEY_NINJA_ARG=-DCMAKE_MAKE_PROGRAM="%shippedNinja%"
exit /b 0


:NO_VS
echo Unable to find Visual Studio with the C++ x64 tools
exit /b 1

:NO_NINJA
echo Unable to find ninja.exe
exit /b 1
