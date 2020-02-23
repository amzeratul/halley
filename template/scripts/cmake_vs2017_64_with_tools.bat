set PreferredToolArchitecture=x64
cd ..
mkdir build
cd build
cmake -G "Visual Studio 15 2017 Win64" ^
    -DHALLEY_PATH=../halley ^
    -DCMAKE_INCLUDE_PATH="lib\include;C:\dev\include;R:\dev\include" ^
    -DCMAKE_LIBRARY_PATH="lib\windows64;C:\dev\lib64;R:\dev\lib64" ^
    -DBOOST_LIBRARYDIR="c:\Boost\lib" ^
    -DBOOST_INCLUDEDIR="c:\Boost\include\boost-1_66" ^
    -DBoost_USE_STATIC_LIBS=1 ^
    -DBUILD_HALLEY_TOOLS=1 ^
    -DBUILD_HALLEY_TESTS=0 ^
    -DUSE_PCH=1 ^
    ..
pause