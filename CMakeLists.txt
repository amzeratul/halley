cmake_minimum_required (VERSION 2.8.10)

project (halley)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin64)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_HOME_DIRECTORY}/bin64)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_HOME_DIRECTORY}/bin64)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake_modules/")

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -stdlib=libc++") # Apparently Clang on Mac needs this...
endif()

if (MSVC)
else()
	set(EXTRA_LIBS pthread)

	find_Package(Freetype REQUIRED)
	find_package(YamlCpp REQUIRED)
	find_Package(SDL2 REQUIRED)
	find_package(OpenGL REQUIRED)
	find_package(GLEW REQUIRED)
	find_package(Boost COMPONENTS system filesystem REQUIRED)
	
	set(FREETYPE_LIB_DEBUG ${FREETYPE_LIB})
	set(YAMLCPP_LIBRARY_DEBUG ${YAMLCPP_LIBRARY})
endif (MSVC)

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
