@echo off

taskkill /IM halley-editor.exe /F 2>NUL
taskkill /IM halley-cmd.exe /F 2>NUL

if "%1"=="--clean" (
    echo Cleaning previous build...
    rmdir /s /q build-ninja 2>NUL
)

if exist halley_deps (
    echo Removing old dependencies...
    rmdir /s /q halley_deps 2>NUL
)

echo Copying DLLs...
mkdir "%~dp0..\bin\" 2>NUL
xcopy "%~dp0..\deps\bin\SDL2.dll" "%~dp0..\bin\" /C /Q /Y
xcopy "%~dp0..\deps\bin\ShaderConductor.dll" "%~dp0..\bin\" /C /Q /Y

set ignoreconsoles=1
if defined NINTENDO_SDK_ROOT (
    echo Nintendo SDK detected at "%NINTENDO_SDK_ROOT%"
    set ignoreconsoles=0
)
if defined GameDK (
    echo Microsoft GDK detected at "%GameDK%"
    set ignoreconsoles=0
)

call "%~dp0setup_ninja_env.bat" || goto ERROR_CONFIGURE

if exist "build-ninja\CMakeCache.txt" goto BUILD
if not exist CMakePresets.json goto CONFIGURE_NO_PRESET

cmake --preset windows-ninja -DHALLEY_IGNORE_CONSOLES=%ignoreconsoles% %HALLEY_NINJA_ARG% || goto ERROR_CONFIGURE
goto BUILD

:CONFIGURE_NO_PRESET
cmake -S . -B build-ninja -G "Ninja Multi-Config" ^
    -DHALLEY_PATH="%~dp0.." ^
    -DBUILD_HALLEY_TOOLS=1 ^
    -DBUILD_HALLEY_TESTS=0 ^
    -DHALLEY_IGNORE_CONSOLES=%ignoreconsoles% ^
    -DCMAKE_INCLUDE_PATH="%~dp0..\deps\include" ^
    -DCMAKE_LIBRARY_PATH="%~dp0..\deps\lib64" ^
    %HALLEY_NINJA_ARG% || goto ERROR_CONFIGURE

:BUILD
REM Try twice
cmake --build build-ninja --target halley-editor --config RelWithDebInfo ^
    || cmake --build build-ninja --target halley-editor --config RelWithDebInfo ^
    || goto ERROR

echo Build successful.
exit /b 0

:ERROR
echo Build failed.
exit /b 1

:ERROR_CONFIGURE
echo CMake configure failed.
exit /b 2
