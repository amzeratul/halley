cd ..
mkdir build
cd build
cmake -G "Visual Studio 16 2019" ^
    -A x64 ^
    -DHALLEY_PATH=../halley ^
    -DCMAKE_INCLUDE_PATH="%~dp0\..\halley_deps\include;lib\include;C:\dev\include;R:\dev\include" ^
    -DCMAKE_LIBRARY_PATH="%~dp0\..\halley_deps\lib64;lib\windows64;C:\dev\lib64;R:\dev\lib64" ^
    -DBOOST_LIBRARYDIR="%~dp0\..\halley_deps\Boost\lib;c:\Boost\lib" ^
    -DBOOST_INCLUDEDIR="%~dp0\..\halley_deps\Boost\include\boost-1_72;c:\Boost\include\boost-1_72" ^
    -DBoost_USE_STATIC_LIBS=1 ^
    -DBUILD_HALLEY_TOOLS=1 ^
    -DBUILD_HALLEY_TESTS=1 ^
    -DBUILD_HALLEY_LAUNCHER=1 ^
    ..
pause