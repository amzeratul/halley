cd ..
mkdir build
cd build
cmake -G "Visual Studio 15 2017 Win64" ^
    -DHALLEY_PATH=../halley ^
    -DBUILD_HALLEY_TOOLS=0 ^
    -DBUILD_HALLEY_TESTS=0 ^
    -DCMAKE_INCLUDE_PATH="lib\include" ^
    -DCMAKE_LIBRARY_PATH="lib\windows64" ^
    -DBOOST_ROOT="lib\boost" ^
    -DBoost_USE_STATIC_LIBS=1 ^
    -DUSE_PCH=1 ^
    ..
pause