#!/bin/sh -e

root=$(pwd)
arch=$(uname -m)
script=$(realpath "$0")
script_path=$(dirname "$script")

mkdir -p build

mkdir -p ${script_path}/../bin
cp ${script_path}/../deps/osx/${arch}/lib/libShaderConductor.dylib libShaderConductor.dylib

#
# generate cmake project
#
cd ${root}/build
rm -f CMakeCache.txt

cmake -G "Xcode" \
  -DHALLEY_PATH="../halley" \
  -DBUILD_HALLEY_TOOLS=1 \
  -DBUILD_HALLEY_TESTS=0 \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DHALLEY_ENABLE_STATIC_STDLIB=1 \
  -DCMAKE_INCLUDE_PATH="${script_path}/../deps/osx/${arch}/include" \
  -DCMAKE_LIBRARY_PATH="${script_path}/../deps/osx/${arch}/lib" \
  -DSDL2_INCLUDE_DIR="${script_path}/../deps/osx/${arch}/include/SDL2" \
	-DSDL2_LIBRARIES="${script_path}/../deps/osx/${arch}/libSDL2.a" \
  -DShaderConductor_INCLUDE_DIR="${script_path}/../deps/osx/${arch}/include/ShaderConductor" \
  -DShaderConductor_LIBRARY="${script_path}/../deps/osx/${arch}/lib/libShaderConductor.dylib" \
  -DBoost_INCLUDE_DIR="${script_path}/../deps/Boost/include/boost-1_81" \
  -DBoost_USE_STATIC_LIBS=1 \
  ..
#
# compile halley-cmd
#
cmake --build . --target halley-cmd --config RelWithDebInfo -j 4

#
# import assets & code gen
#
cd ${script_path}/../bin
./halley-cmd import ${root} ${root}/halley/

#
# compile halley-editor
#
cd ${root}/build
cmake --build . --target halley-editor --config RelWithDebInfo -j 4
