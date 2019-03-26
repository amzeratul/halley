# Building your Halley Project
## Requirements
Halley is written in **C++14**. It builds on **Windows** (32 and 64), **MacOS**, and **Linux**. It requires **Visual Studio 2017** (Visual Studio 2015 is temporarily supported) on Windows, and a reasonably up to date **gcc** or **clang** on the other platforms.

A recent version (3.10+) of CMake is required to build the project.

Halley also requires the following dependencies:
* Boost 1.66.0+ (headers only unless building tools)
* libogg
* libvorbis
* Lua 5.3
* SDL 2.0.2 (recomended: 2.0.7+) (only for PC ports)
* yaml-cpp 0.5.3+ (if building tools)
* Freetype 2.6.3+ (if building tools)
* Recent version of Windows 10 SDK (only for Windows)

## Codegen

To build a Halley project, you need to run codegen on it first, or it will fail. This is part of the [[Importing Assets]] stage. However, if you're building your project and Halley on the same go, and you're building the Halley tools needed for importing the assets, you might have to build first (so the tools build and the game fails), then import, then build again.

## Invoking CMake

It's recommended that a folder called **build** is created on the root of your repo, and **CMake** is invoked from there ("out of tree" build).

If building **without Halley tools**, some variant of this should work:

```batch
cmake -G "Visual Studio 15 2017 Win64" -DHALLEY_PATH=../halley -DBUILD_HALLEY_TOOLS=0 -DBUILD_HALLEY_TESTS=0 -DCMAKE_INCLUDE_PATH="path\to\include" -DCMAKE_LIBRARY_PATH="path\to\libs" -DBOOST_ROOT="path\to\boost" -DBoost_USE_STATIC_LIBS=1 ..
```

If building **with Halley tools**, then you'll need to enable that flag, and help CMake find boost, for example:

```batch
cmake -G "Visual Studio 15 2017 Win64" -DHALLEY_PATH=../halley -DBUILD_HALLEY_TOOLS=1 -DBUILD_HALLEY_TESTS=0 -DCMAKE_INCLUDE_PATH="path\to\include" -DCMAKE_LIBRARY_PATH="path\to\libs" -DBoost_USE_STATIC_LIBS=1 -DBOOST_LIBRARYDIR="c:\Boost\lib" -DBOOST_INCLUDEDIR="c:\Boost\include\boost-1_66" ..
```

## Build!

If everything went well, then you've probably managed to get through the hardest part, so you should be able to just build the project now!

# Extra information
## Building Boost

As a reference for users on Windows, this is how Boost is typically built:

1. Download the latest Boost from boost.org and uncompress it somewhere
1. Launch the Visual Studio "x64 Native Tools Command Prompt for VS2017" (or equivalent, it will be on your start menu)
1. Inside this command prompt window, navigate to the root of the extracted boost distro (where bootstrap.bat is located)
1. Run `bootstrap.bat msvc`
1. Run `b2 link=static threading=multi runtime-link=shared address-model=64 -j8 install`
1. For some reason, CMake looks for the boost libraries without the -x64 part of the filename. You'll have to rename the **filesystem** and **system** libraries (and any other that might be added in the future, if this is out of date) to remove that bit, or CMake will fail, stating it can't find those.

## Using BCP
If you want to collect the stripped down Boost headers for distribution with your project, at the time of writing, this should contain everything that Halley uses:

`bcp asio optional container variant pool error_code stacktrace c:\path\to\output`

bcp is a tool provided in the Boost distro, and can be compiled by simply running **b2** on its directory.