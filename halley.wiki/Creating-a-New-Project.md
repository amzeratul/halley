# Setup the folder structure
Create a new repository for your project, and follow this directory structure:

```
 /
 |
 +-- .gitignore
 +-- CMakeLists.txt
 +-- /assets
 |  - [generated files go here]
 +-- /assets_src 
 |  - YOUR ASSETS HERE
 +-- /bin
 |  - [where your binaries will go]
 +-- /build
 |  - [where your CMake build files will go]
 +-- /gen
 |  - [generated files go here]
 +-- /gen_src
 |  - YOUR CODEGEN SOURCE HERE
 +-- /halley
 |  - submodule or clone of the halley repo
 +-- /halley-external
 |   - submodules or clones of external halley modules (e.g. console ports)
 +-- /plugins
 |   - plugins for the editor can go here
 +-- /src
     - YOUR GAME SOURCE
```

# Setup CMakeLists.txt
Your CMakeLists.txt should look something like this:

```cmake
cmake_minimum_required (VERSION 3.10)

project (project_name)

set(HALLEY_PROJECT_EMBED 1)
set(HALLEY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/halley)
set(HOTRELOAD 0 CACHE BOOL "Sets whether to hot-reload this project")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${HALLEY_PATH}/cmake/")

include(HalleyProject)

set (SOURCES
     # sources here
     )

set (HEADERS
     # headers here
     )

halleyProject(project_name "${SOURCES}" "${HEADERS}" "" ${CMAKE_CURRENT_SOURCE_DIR}/bin)
```

# Setup .gitignore
A minimum recommended .gitignore file is:

```
assets/
bin/
build/
build-external/
gen/
```

# Build
For the next steps, see [[Building]] and [[Importing Assets]]