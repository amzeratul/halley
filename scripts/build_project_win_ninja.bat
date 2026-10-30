@echo off

REM Usage: build_project_win_ninja.bat <projectDir> <binName> <config>

set "projectDir=%~1"
set "binName=%~2"
set "config=%~3"

cd /d "%projectDir%" || exit /b 1

taskkill /IM "%binName%.exe" /F >nul 2>nul

if not exist bin mkdir bin
if not exist "bin\SDL2.dll" (
    echo Copying SDL2.dll to bin
    copy "%~dp0..\deps\bin\SDL2.dll" "bin\" || exit /b 1
)

call "%~dp0setup_ninja_env.bat" || exit /b 1

if exist "build-ninja\CMakeCache.txt" goto BUILD
echo Configuring Ninja...
if not exist CMakePresets.json goto CONFIGURE_NO_PRESET

cmake --preset windows-ninja %HALLEY_NINJA_ARG% || exit /b 1
goto BUILD

:CONFIGURE_NO_PRESET
cmake -S . -B build-ninja -G "Ninja Multi-Config" ^
    -DHALLEY_PATH="%projectDir%\halley" ^
    -DBUILD_HALLEY_TOOLS=0 ^
    -DBUILD_HALLEY_TESTS=0 ^
    -DCMAKE_INCLUDE_PATH="%projectDir%\lib\include" ^
    -DCMAKE_LIBRARY_PATH="%projectDir%\lib\windows64" ^
    %HALLEY_NINJA_ARG% || exit /b 1

:BUILD
echo Building with Ninja...
cmake --build build-ninja --target "%binName%-gamebins" --config %config% || exit /b 1

echo Build successful.
