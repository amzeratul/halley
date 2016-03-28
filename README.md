![Halley Logo](http://higherorderfun.com/stuff/halley/halley2.png)

# Halley Game Engine
A lightweight game engine written in modern C++. I'm currently re-architecting the whole engine, while experimenting with some crazy ideas, so it's not usable at all right now (most of the engine is not even there!).

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

### Dependencies
* CMake 3.0+
* Haskell Platform (to build codegen)
* Visual Studio 2015
* Boost 1.59.0+
* SDL 2.0.4+

### Set up
* Make sure that SDL and Boost are accessible.
* Compile codegen by running "cabal build" on its folder. You may need to download and install dependencies.
* Add [repo]/include and [repo]/lib/x64 to your global include and library lookup paths, respectively.
* Open halley.sln [Soon CMake will replace this]
* Compile!
