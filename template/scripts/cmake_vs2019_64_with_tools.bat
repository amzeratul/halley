cd ..
mkdir build
cd build
cmake -G "Visual Studio 16 2019" ^
    -A x64 ^
    -DHALLEY_PATH=../halley ^
    -DCMAKE_INCLUDE_PATH="%~dp0\..\halley_deps\include;lib\include" ^
    -DCMAKE_LIBRARY_PATH="%~dp0\..\halley_deps\lib64" ^
    -DBUILD_HALLEY_TOOLS=1 ^
    -DBUILD_HALLEY_TESTS=1 ^
    -DBUILD_HALLEY_LAUNCHER=1 ^
    ..
pause