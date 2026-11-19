![Halley Logo](http://higherorderfun.com/stuff/halley/halley_scarlet.png)

[![License](https://img.shields.io/badge/license-Apache%202.0-brightgreen.svg)](https://en.wikipedia.org/wiki/C%2B%2B14)
[![Language](https://img.shields.io/badge/language-C++-blue.svg)](https://isocpp.org/)
[![Standard](https://img.shields.io/badge/c%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)

# Halley Game Engine
A lightweight game engine written in C++17. It has been used to ship Wargroove, a turn-based strategy game, on Windows, Mac (experimental), Nintendo Switch, Xbox One and PS4 (with experimental Android and iOS ports WIP).

The Halley Game Engine is licensed under the Apache 2.0 license.

Check out [this repo](https://github.com/amzeratul/ggj20) for a sample game (our Global Game Jam 2020 entry).

Join us on our [Discord server](https://discord.gg/T7qQqQJ)!

## Design guidelines & objectives
* Modern graphics pipelines, with first-class shader support
* Written in modern C++
* "True" entity system based, with components that store data and systems that operate on families of components
* Tuned for high performance
* Code-first: no reliance on editor to generate anything
* Easy to use: games should be easy to implement
* Easy to maintain: games should be easy to keep working on long-term, after it has grown to a large project size
* Cross-platform: support as many platforms as possible
* No legacy: don't bother supporting legacy systems, such as Windows XP or older Android/iOS devices
* Rich tools: where possible, provide tools to assist in development and debugging
* Fast iteration time: hot-reloading wherever possible
* Support scripting in other languages, such as Lua

## Modules
Halley is divided in a several sub-projects:
* **engine**
  * **core**: Core features of the engine, including looper, API management, resources, and graphics engine
  * **audio**: Audio engine
  * **entity**: Framework for dealing with entities, components, and systems
  * **utils**: Utilities library
  * **net**: Networking library
  * **ui**: UI library
* **plugins**
  * **asio**: Network/Asio implementation
  * **dx11**: Video/DX11 implementation
  * **opengl**: Video/OpenGL implementation
  * **metal**: Video/Metal implementation (experimental)
  * **sdl**: System/SDL, Input/SDL and AudioOut/SDL implementations
  * **winrt**: System/WinRT, Input/WinRT, Platform/WinRT and AudioOut/XAudio2 implementations
* **tools**
  * **editor**: Editor UI
  * **cmd**: Command-line interface to tools
  * **runner**: Provides an entry point for execution and dynamic reloading. Highly experimental.
  * **tools**: Editor tools to generate files and assets
* **tests**
  * **entity**: Stress test of entity system
  * **network**: Stress test of network system
* **samples**
  * (The samples project was taken down due to being too outdated, sorry about that!)

## Platforms
The following platforms are supported:
* **Windows**: Tested on Windows 10 Professional 64-bit (Might work on as low as XP 32-bit, but XP is no longer a tested target)
* **macOS**: Tested on Mac OS X 10.9.6 (Intel); also builds and runs natively on Apple Silicon (arm64) — see [macOS (Apple Silicon)](#macos-apple-silicon--arm64) below
* **Linux**: Tested on Ubuntu 16.04

## Installation

### Tools required
* CMake 3.10+
* C++17 capable compiler:
  * Visual C++ 15.9 (Visual Studio 2017)
  * Clang 5
  * GCC 7

### Library dependencies
* Engine:
  * OpenGL [optional]
  * SDL 2.0.2 (2.0.7 recommended) [optional]
  * Windows 10 SDK [optional]
* Also required if building Tools:
  * Freetype 2.6.3
  * yaml-cpp 0.5.3

### Set up
* Ensure that all dependencies above are set up correctly
* Build with CMake
  * Typical:  
     ```
     cmake -DCMAKE_INCLUDE_PATH=path/to/headers \
           -DCMAKE_LIBRARY_PATH=path/to/libs \
           ..
     ```
  * Engine only:
    ```
    cmake -DBUILD_HALLEY_TOOLS=0 -DBUILD_HALLEY_TESTS=0 [...]
    ```
* Run `halley-editor tests/entity` (or whichever other project you want to test)
* Launch that project

### macOS (Apple Silicon / arm64)
The bundled `deps/osx` libraries are x86_64-only, so a native arm64 build pulls current dependencies from [Homebrew](https://brew.sh) instead:
```
brew install freetype sdl2 googletest openssl
```
(`yaml-cpp` is vendored under `src/contrib`, so its Homebrew package is optional.)

[ShaderConductor](https://github.com/microsoft/ShaderConductor) (required for the tools/editor) has no arm64 prebuilt, so build it from source. Its pinned DXC/LLVM fork needs a couple of flags to compile under recent Apple Clang — configure it with `-DCMAKE_CXX_FLAGS=-Wno-invalid-specialization -DSPIRV_WERROR=OFF`, and drop the `-march=core2 -msse2` / `-Werror` lines that `Source/CMakeLists.txt` forces for non-arm hosts. This yields an arm64 `libShaderConductor.dylib`.

Configure Halley with Ninja, pointing CMake at the Homebrew prefix and your ShaderConductor build. The explicit `SDL2_*` paths matter, otherwise CMake picks up the Windows-configured SDL2 headers under `deps/include`:
```
cmake -G Ninja \
      -DBUILD_HALLEY_TOOLS=1 -DBUILD_HALLEY_TESTS=1 \
      -DCMAKE_PREFIX_PATH=/opt/homebrew \
      -DSDL2_INCLUDE_DIR=/opt/homebrew/include/SDL2 \
      -DSDL2_LIBRARIES=/opt/homebrew/lib/libSDL2.dylib \
      -DShaderConductor_INCLUDE_DIR="<ShaderConductor>/Include;<ShaderConductor>/Include/ShaderConductor" \
      -DShaderConductor_LIBRARY=<ShaderConductor>/Build/.../Lib/libShaderConductor.dylib \
      ..
cmake --build . --config RelWithDebInfo
```
Then launch the editor with `./bin/halley-editor --dont-load-dll`.

## Documentation
The full documentation is available on the [Wiki](https://github.com/amzeratul/halley/wiki).
