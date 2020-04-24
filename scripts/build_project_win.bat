@echo off

cd /d %1
if not exist build mkdir build
cd build

set slnName="%~2.sln"
set binTargetName="%~2-gamebins"

IF NOT EXIST %slnName% (
cmake -G "Visual Studio 16 2019" ^
    -A x64 ^
    -DHALLEY_PATH=../halley ^
    -DBUILD_HALLEY_TOOLS=0 ^
    -DBUILD_HALLEY_TESTS=0 ^
    -DCMAKE_INCLUDE_PATH="lib\include" ^
    -DCMAKE_LIBRARY_PATH="lib\windows64" ^
    -DBOOST_ROOT="lib\boost" ^
    -DBoost_USE_STATIC_LIBS=1 ^
    -DUSE_PCH=1 ^
    .. || exit /b 1
)

cmake.exe --build . --target %binTargetName% --config RelWithDebInfo || exit /b 1