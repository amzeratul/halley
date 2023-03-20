@echo off

taskkill /IM halley-editor.exe /F 2>NUL
taskkill /IM halley-cmd.exe /F 2>NUL

if "%1"=="--clean" (
    echo Cleaning previous build...
    rmdir /s /q build 2>NUL
)

if exist halley_deps (
    echo Removing old dependencies...
    rmdir /s /q halley_deps 2>NUL
)

if not exist build mkdir build
cd build

echo Copying DLLs...
mkdir "%~dp0..\bin\" 2>NUL
xcopy "%~dp0..\deps\bin\SDL2.dll" "%~dp0..\bin\" /C /Q /Y
xcopy "%~dp0..\deps\bin\dxcompiler.dll" "%~dp0..\bin\" /C /Q /Y
xcopy "%~dp0..\deps\bin\ShaderConductor.dll" "%~dp0..\bin\" /C /Q /Y

set ignoreconsoles=1
if defined NINTENDO_SDK_ROOT (
    echo Nintendo SDK detected
    set ignoreconsoles=0
)

cmake -A x64 ^
    -DHALLEY_PATH=../halley ^
    -DBUILD_HALLEY_TOOLS=1 ^
    -DBUILD_HALLEY_TESTS=0 ^
    -DHALLEY_IGNORE_CONSOLES=%ignoreconsoles% ^
    -DCMAKE_INCLUDE_PATH="%~dp0..\deps\include" ^
    -DCMAKE_LIBRARY_PATH="%~dp0..\deps\lib64" ^
    -DBOOST_INCLUDEDIR="%~dp0..\deps\Boost\include\boost-1_81" ^
    .. || goto ERROR_CONFIGURE

cmake.exe --build . --target halley-cmd --config RelWithDebInfo || goto ERROR
cmake.exe --build . --target halley-editor --config RelWithDebInfo || goto ERROR

echo Build successful.
exit /b 0

:ERROR
echo Build failed.
exit /b 1

:ERROR_CONFIGURE
echo CMake configure failed.
exit /b 2
