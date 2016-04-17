![Halley Logo](http://higherorderfun.com/stuff/halley/halley2.png)

# Halley Game Engine
A lightweight game engine written in modern C++. I'm currently re-architecting the whole engine, while experimenting with some crazy ideas, so it's not usable at all right now (some basic components, such as input, are not there at all).

Design guidelines:
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

## Modules
Halley is divided in a few sub-projects:
* **core**: Core features of the engine, including looper, API management
* **opengl**: OpenGL backend
* **entity**: Framework for dealing with entities, components, and systems
* **codegen**: Generates code for the entity system (written in Haskell)
* **runner**: Provides an entry point for execution and dynamic reloading. Highly experimental.
* **utils**: Utilities library
* **samples/test**: Sample game

## Installation

### Tools
* Visual C++ 14 Update 2 (Visual Studio 2015)
* Haskell Platform 7.10.3 (to build codegen)
* CMake 3.x

### Dependencies
* Boost 1.59.0 (should we get rid of this?)
* SDL 2.0.4
* GLEW 1.13.0
* yaml-cpp 0.5.3

### Set up
* Make sure that all dependencies are installed and accessible to the compiler
* Compile codegen by running "cabal build" on its folder. You may need to download and install dependencies.
* Add [repo]/include and [repo]/lib/x64 to your global include and library lookup paths, respectively.
* Open halley.sln [Soon CMake will replace this]
* Compile!
