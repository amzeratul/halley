![Halley Logo](http://higherorderfun.com/stuff/halley/halley_scarlet.png)

[![License](https://img.shields.io/badge/license-Apache%202.0-brightgreen.svg)](https://en.wikipedia.org/wiki/C%2B%2B14)
[![Language](https://img.shields.io/badge/language-C++-blue.svg)](https://isocpp.org/)
[![Standard](https://img.shields.io/badge/c%2B%2B-14-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B14)

# Halley Game Engine
A lightweight game engine written in C++14. It has been used to ship Wargroove, a turn-based strategy game, on Windows, Mac (experimental), Nintendo Switch and Xbox One (soon PS4!).

The Halley Game Engine is licensed under the Apache 2.0 license.

Make sure to check out the [Halley-template](https://github.com/amzeratul/halley-template) repository for a blank sample.

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
  * **core**: Core features of the engine, including looper, API management, resouces, and graphics engine
  * **audio**: Audio engine
  * **entity**: Framework for dealing with entities, components, and systems
  * **utils**: Utilities library
  * **net**: Networking library
  * **ui**: UI library
* **plugins**
  * **asio**: Network/Asio implementation
  * **dx11**: Video/DX11 implementation
  * **opengl**: Video/OpenGL implementation
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
* **Windows**: Tested on Windows 10 Professional 64-bit (Should work on as low as XP 32-bit)
* **Mac OS X**: Tested on Mac OS X 10.9.6
* **Linux**: Tested on Ubuntu 16.04

## Installation

### Containers

If you want to quickly jump in, you can pull the provided [Dockerfile](Dockerfile)
(that builds a Docker container) that comes with the dependencies installed below.

If you are good with Docker you can try the container (you will need to expose the video
device from your host)

```bash
$ docker pull vanessa/halley
```

But easier is to pull the Docker container into a [Singularity](https://www.sylabs.io/guides/3.0/user-guide/installation.html) container. 
This will avoid dealing with trying to get our video devices to work in the Docker container!

```bash
$ sudo singularity build --sandbox halley-box docker://vanessa/halley
$ singularity shell halley-box
```

Once in the container, try the test:

```bash
$ cd /code/src/tests/
$ halley-editor entity/
```

And the display should work:

![assets_src/image/entity.png](assets_src/image/entity.png)


### Local

#### Tools required
* CMake 3.10+
* C++14 capable compiler:
  * Visual C++ 15.3 (Visual Studio 2017)
  * Clang 3.5
  * GCC 5.0

#### Library dependencies
* Engine:
  * Boost 1.66.0 (header only)
  * OpenGL [optional]
  * SDL 2.0.2 (2.0.7 recommended) [optional]
  * Windows 10 SDK [optional]
* Also required if building Tools:
  * Boost 1.66.0 libraries: filesystem, system
  * Freetype 2.6.3
  * yaml-cpp 0.5.3

#### Set up
* Ensure that all dependencies above are set up correctly
* Build with CMake
  * Typical: "cmake -DCMAKE_INCLUDE_PATH=path/to/headers -DCMAKE_LIBRARY_PATH=path/to/libs -DBOOST_ROOT=path/to/boost .."
  * Engine only: "cmake -DBUILD_HALLEY_TOOLS=0 -DBUILD_HALLEY_TESTS=0 [...]"
* Run "halley-editor tests/entity" (or whichever other project you want to test)
* Launch that project

## Documentation
The full documentation is available on the [Wiki](https://github.com/amzeratul/halley/wiki).
