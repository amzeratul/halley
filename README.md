![Halley Logo](http://higherorderfun.com/stuff/halley/halley_scarlet.png)

# Halley Game Engine
A lightweight game engine written in C++14. I'm currently re-architecting the whole engine, while experimenting with some crazy ideas, so it's not usable at all right now (some basic components, such as input, are not there at all).

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
* Support scripting in other languages, such as Lua

## Modules
Halley is divided in a few sub-projects:
* **core**: Core features of the engine, including looper, API management
* **opengl**: OpenGL backend
* **entity**: Framework for dealing with entities, components, and systems
* **runner**: Provides an entry point for execution and dynamic reloading. Highly experimental.
* **utils**: Utilities library
* **tools**: Editor tools to generate files and assets
* **cmd**: Command-line interface to tools
* **samples/test**: Sample game

## Installation

### Tools
* CMake 3.x
* C++14 capable compiler:
** Visual C++ 14 Update 2 (Visual Studio 2015)
** Clang 3.5
** GCC 5.0

### Dependencies
* Boost 1.59.0 (Can we get rid of this? Only two files in Utils use it)
* SDL 2.0.4
* GLEW 1.13.0
* yaml-cpp 0.5.3
* Freetype 2.6.3

### Set up
* Run CMake
