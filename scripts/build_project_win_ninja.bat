@echo off


cd /d %1
if not exist build-ninja mkdir build-ninja
cd build-ninja


IF EXIST "vcvars.bat" GOTO RUNVCVARS

for /f "usebackq tokens=1* delims=: " %%i in (`"%~1/halley/bin/vswhere.exe" -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64`) do (
    if /i "%%i"=="installationPath" set vsdir=%%j
)
if defined %vsdir% (echo Visual Studio at %vsdir%) else (echo Unable to find Visual Studio && exit /b 1)
@echo call "%vsdir%\VC\Auxiliary\Build\vcvars64.bat" ^|^| exit /b 1 > vcvars.bat


:RUNVCVARS
call vcvars.bat || exit /b 1


echo Configuring Ninja...
IF NOT EXIST "build.ninja" (
cmake -GNinja ^
    -DHALLEY_PATH=../halley ^
    -DBUILD_HALLEY_TOOLS=0 ^
    -DBUILD_HALLEY_TESTS=0 ^
    -DCMAKE_INCLUDE_PATH="lib\include" ^
    -DCMAKE_LIBRARY_PATH="lib\windows64" ^
    -DBOOST_ROOT="lib\boost" ^
    -DBoost_USE_STATIC_LIBS=1 ^
    -DCMAKE_BUILD_TYPE=%3 ^
    .. || exit /b 1
)

set targetDllPath="%~1\bin\SDL2.dll"
if not exist %targetDllPath% (
    echo Copying SDL2.dll to %targetDllPath%
    copy SDL2.dll %targetDllPath% || exit /b 1
)

set binTargetName="%~2-gamebins"
echo Building with Ninja...
cmake.exe --build . --target %binTargetName% --config %3 || exit /b 1

echo Build successful.
