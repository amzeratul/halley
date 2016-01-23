# Halley Game Engine
A lightweight game engine written in modern C++. I'm currently re-architecting the whole engine, while experimenting with some crazy ideas, so it's not usable at all right now (most of the engine is not even there!).

## Modules
Halley is divided in a few sub-projects:
* **core**: Core features of the engine, such as input, graphics, audio, etc
* **entity**: Framework for dealing with entities, components, and systems
* **codegen**: Generates code for the entity system (written in Haskell)
* **runner**: Provides an entry point for execution and dynamic reloading
* **utils**: Utilities library
* **sample**: Sample game

## Installation

### Dependencies
* Haskell Platform (to build codegen)
* Visual Studio 2015
* Rest: TODO

### Set up
* Compile codegen by running "cabal build" on its folder.
* Rest: TODO :D
