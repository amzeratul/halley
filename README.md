![Halley Logo](http://higherorderfun.com/stuff/halley/halley_scarlet.png)

[![Language](https://img.shields.io/badge/language-C++-blue.svg)](https://isocpp.org/)
[![Standard](https://img.shields.io/badge/c%2B%2B-14-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B14)

# Halley Game Engine
A lightweight game engine written in C++14. I'm currently re-architecting the whole engine, while experimenting with some crazy ideas, so it's not fully usable yet.

Design guidelines & objectives:
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
  * **core**: Core features of the engine, including looper, API management
  * **opengl**: Video/OpenGL implementation
  * **sdl**: System/SDL implementation
  * **entity**: Framework for dealing with entities, components, and systems
  * **utils**: Utilities library
  * **net**: Networking library
* **tools**
  * **editor**: Editor UI
  * **cmd**: Command-line interface to tools
  * **runner**: Provides an entry point for execution and dynamic reloading. Highly experimental.
  * **tools**: Editor tools to generate files and assets
* **tests**
  * **entity**: Stress test of entity system
  * **network**: Stress test of network system

## Installation

### Tools
* CMake 3.x
* C++14 capable compiler:
  * Visual C++ 14 Update 2 (Visual Studio 2015)
  * Clang 3.5
  * GCC 5.0

### Dependencies
* Engine:
  * Boost 1.59.0
  * SDL 2.0.4 (technically only for SDL system, but no alternative is available)
  * yaml-cpp 0.5.3
* Tools only:
  * Freetype 2.6.3

### Set up
* Build with CMake
* Run halley-editor
