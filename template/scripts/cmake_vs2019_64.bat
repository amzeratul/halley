cd ..
mkdir build
cd build
cmake -G "Visual Studio 16 2019" ^
    -A x64 ^
    -DHALLEY_PATH=../halley ^
    -DBUILD_HALLEY_TOOLS=0 ^
    -DBUILD_HALLEY_TESTS=0 ^
    -DCMAKE_INCLUDE_PATH="lib\include" ^
    -DCMAKE_LIBRARY_PATH="lib\windows64" ^
    -DBOOST_ROOT="lib\boost" ^
    -DBoost_USE_STATIC_LIBS=1 ^
    ..
pause