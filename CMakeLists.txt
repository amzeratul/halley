cmake_minimum_required (VERSION 2.8.10)

project (halley)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin64)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_HOME_DIRECTORY}/bin64)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_HOME_DIRECTORY}/bin64)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

SET(HALLEY_VERSION_MAJOR "2")
SET(HALLEY_VERSION_MINOR "0")
SET(HALLEY_VERSION_PATCH "0")
SET(HALLEY_VERSION "${HALLEY_VERSION_MAJOR}.${HALLEY_VERSION_MINOR}.${HALLEY_VERSION_PATCH}")

include(cmake_modules/PrecompiledHeader.cmake)

add_subdirectory(utils)
add_subdirectory(entity)
add_subdirectory(opengl)
add_subdirectory(tools)
add_subdirectory(cmd)
add_subdirectory(core)
add_subdirectory(samples)
