include(PrecompiledHeader)


function(string_starts_with str prefix var)
  string(LENGTH "${str}" str_length)
  string(LENGTH "${prefix}" prefix_length)
  set(value FALSE)
  if(NOT ${str_length} LESS ${prefix_length})
    string(SUBSTRING "${str}" 0 ${prefix_length} str_prefix)
    if("${str_prefix}" STREQUAL "${prefix}")
      set(value TRUE)
    endif()
  endif()
  set(${var} ${value} PARENT_SCOPE)
endfunction()


set(DEV_BUILD 1)


# Gitlab CI support
set(VERSION_FILE_DATA "")
if (DEFINED ENV{CI_PIPELINE_ID})
	set(VERSION_FILE_DATA "${VERSION_FILE_DATA} #define VERSION_BUILD $ENV{CI_PIPELINE_ID}\n")
else()
	set(VERSION_FILE_DATA "${VERSION_FILE_DATA} #define VERSION_BUILD 0\n")
endif()
if (DEFINED ENV{CI_COMMIT_SHA})
	set(VERSION_FILE_DATA "${VERSION_FILE_DATA} #define COMMIT_SHA \"$ENV{CI_COMMIT_SHA}\"\n")
else()
	set(VERSION_FILE_DATA "${VERSION_FILE_DATA} #define COMMIT_SHA \"\"\n")
endif()
if (DEFINED ENV{CI_COMMIT_REF_SLUG})
	set(VERSION_FILE_DATA "${VERSION_FILE_DATA} #define BRANCH_NAME \"$ENV{CI_COMMIT_REF_SLUG}\"\n")
	set(REF_SLUG $ENV{CI_COMMIT_REF_SLUG})
	string_starts_with(${REF_SLUG} "release" IS_RELEASE)
	if (IS_RELEASE)
		set(DEV_BUILD 0)
	endif()
else()
	set(VERSION_FILE_DATA "${VERSION_FILE_DATA} #define BRANCH_NAME \"\"\n")
endif()
file(WRITE gen/cpp/build_version.h "${VERSION_FILE_DATA}\n")

if (DEV_BUILD EQUAL 1)
	add_definitions(-DDEV_BUILD)
endif()


# C++14 support
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -stdlib=libc++") # Apparently Clang on Mac needs this...
endif()
if (EMSCRIPTEN)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -stdlib=libc++")
	add_definitions(-s USE_SDL=2)
endif()


# Compiler-specific flags
if (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /fp:fast")
	if (MSVC_VERSION GREATER_EQUAL 1910)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17 /permissive-")
	endif ()
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /sdl /Oi /Ot /Oy /Ob2 /Zi")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /sdl /Oi /Ot /Oy /Ob2 /Zi")
	#set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG")

	if(DEV_BUILD EQUAL 0)
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GL")
		set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /GL")
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
		set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /LTCG")
	endif()

	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG:FASTLINK")
	set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS_RELEASE} /LTCG")
	set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG")
	set(CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO} /LTCG")
	set(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO} /LTCG")
	set(CMAKE_CXX_STANDARD 17)

	if (${CMAKE_SYSTEM_NAME} MATCHES "WindowsStore")
		add_definitions(-D_WIN32_WINNT=0x0A00 -DWINVER=0x0A00 -DWINDOWS_STORE)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /await")
	else()
		add_definitions(-D_WIN32_WINNT=0x0601 -DWINVER=0x0601)
	endif()
	add_definitions(-D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
else()
	if (NOT EMSCRIPTEN)
		set(EXTRA_LIBS pthread)
	endif()
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")

	if (HALLEY_ENABLE_STATIC_STDLIB)
		if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU") 
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
		endif()
	endif()
endif()


# GSL flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DGSL_THROW_ON_CONTRACT_VIOLATION")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DGSL_UNENFORCED_ON_CONTRACT_VIOLATION")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DGSL_UNENFORCED_ON_CONTRACT_VIOLATION")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -DGSL_UNENFORCED_ON_CONTRACT_VIOLATION")


# Pick dependencies
set(USE_OPENGL 1)
set(USE_DX11 0)
set(USE_SDL2 1)
set(USE_ASIO 1)
set(USE_WINRT 0)
set(USE_MEDIA_FOUNDATION 0)

if (EMSCRIPTEN)
	set(USE_SDL2 0)
	set(USE_ASIO 0)
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set(USE_DX11 1)
	set(USE_MEDIA_FOUNDATION 1)
endif ()

if (${CMAKE_SYSTEM_NAME} MATCHES "WindowsStore")
	set(USE_DX11 1)
	set(USE_MEDIA_FOUNDATION 1)
	set(USE_SDL2 0)
	set(USE_OPENGL 0)
	set(USE_ASIO 0)
	set(USE_WINRT 1)
endif ()


# Libs
if (CMAKE_LIBRARY_PATH)
	link_directories(${CMAKE_LIBRARY_PATH})
endif()

# SDL2
if (USE_SDL2)
	add_definitions(-DWITH_SDL2)
	if(EMSCRIPTEN)
		set(SDL2_INCLUDE_DIR "")
		set(SDL2_LIBRARIES "")
	else()
		set (SDL2_BUILDING_LIBRARY 1)
		find_Package(SDL2 REQUIRED)
	endif()
endif()

# Boost
find_package(Boost REQUIRED)
add_definitions(-DBOOST_ALL_NO_LIB -DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE)
if (BOOST_INCLUDE_DIR)
else()
	set(BOOST_INCLUDE_DIR "${Boost_INCLUDE_DIRS}")
endif()

# OpenGL
if (USE_OPENGL)
	add_definitions(-DWITH_OPENGL)
	find_package(OpenGL REQUIRED)
	if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
		find_library(X11 REQUIRED)
	else()
		set(X11_LIBRARIES "")
	endif ()
endif ()

# Asio
if (USE_ASIO)
	add_definitions(-DWITH_ASIO)
endif ()

# DX11
if (USE_DX11)
	add_definitions(-DWITH_DX11)
endif()

# WinRT
if (USE_WINRT)
	add_definitions(-DWITH_WINRT)
endif()

# Microsoft Media Foundation
if (USE_MEDIA_FOUNDATION)
	add_definitions(-DWITH_MEDIA_FOUNDATION)
endif()


# Apple frameworks
if (APPLE)
	find_library(CARBON_LIBRARY Carbon)
	find_library(COCOA_LIBRARY Cocoa)
	find_library(COREAUDIO_LIBRARY CoreAudio)
	find_library(AUDIOUNIT_LIBRARY AudioUnit)
	find_library(FORCEFEEDBACK_LIBRARY ForceFeedback)
	find_library(IOKIT_LIBRARY IOKit)
	find_library(COREVIDEO_LIBRARY CoreVideo)
	find_library(AUDIOTOOLBOX_LIBRARY AudioToolbox)

	mark_as_advanced(CARBON_LIBRARY COCOA_LIBRARY COREAUDIO_LIBRARY AUDIOTOOLBOX_LIBRARY AUDIOUNIT_LIBRARY FORCEFEEDBACK_LIBRARY IOKIT_LIBRARY COREVIDEO_LIBRARY)

	set(EXTRA_LIBS ${EXTRA_LIBS} ${CARBON_LIBRARY} ${COCOA_LIBRARY} ${COREAUDIO_LIBRARY} ${AUDIOTOOLBOX_LIBRARY} ${AUDIOUNIT_LIBRARY} ${FORCEFEEDBACK_LIBRARY} ${IOKIT_LIBRARY} ${COREVIDEO_LIBRARY} iconv)
endif(APPLE)

# From http://stackoverflow.com/questions/31422680/how-to-set-visual-studio-filters-for-nested-sub-directory-using-cmake
function(assign_source_group)
	foreach(_source IN ITEMS ${ARGN})
		if (IS_ABSOLUTE "${_source}")
			file(RELATIVE_PATH _source_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_source}")
		else()
			set(_source_rel "${_source}")
		endif()
		get_filename_component(_source_path "${_source_rel}" PATH)
		string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
		source_group("${_source_path_msvc}" FILES "${_source}")
	endforeach()
endfunction(assign_source_group)


# Setup libraries
set(CMAKE_DEBUG_POSTFIX "_d")

set(HALLEY_PROJECT_EXTERNAL_LIBS
	${SDL2_LIBRARIES}
	${OPENGL_LIBRARIES}
	${X11_LIBRARIES}
	${EXTRA_LIBS}
	)

set(HALLEY_PROJECT_LIBS
	optimized halley-ui
	optimized halley-core
	optimized halley-entity
	optimized halley-audio
	optimized halley-net
	optimized halley-lua
	optimized halley-utils
	debug halley-ui_d
	debug halley-core_d
	debug halley-entity_d
	debug halley-audio_d
	debug halley-net_d
	debug halley-lua_d
	debug halley-utils_d
	${HALLEY_PROJECT_EXTERNAL_LIBS}
	)

if (USE_DX11)
	set(HALLEY_PROJECT_LIBS
		optimized halley-dx11
		debug halley-dx11_d
		${HALLEY_PROJECT_LIBS}
		)
endif ()

if (USE_MEDIA_FOUNDATION)
	set(HALLEY_PROJECT_LIBS
		optimized halley-mf
		debug halley-mf_d
		${HALLEY_PROJECT_LIBS}
		)
endif ()

if (USE_SDL2)
set(HALLEY_PROJECT_LIBS
	optimized halley-sdl
	debug halley-sdl_d
	${HALLEY_PROJECT_LIBS}
	)
endif ()

if (USE_ASIO)
set(HALLEY_PROJECT_LIBS
	optimized halley-asio
	debug halley-asio_d
	${HALLEY_PROJECT_LIBS}
	)
endif ()

if (USE_OPENGL)
set(HALLEY_PROJECT_LIBS
	optimized halley-opengl
	debug halley-opengl_d
	${HALLEY_PROJECT_LIBS}
	)
endif ()

if (USE_WINRT)
set(HALLEY_PROJECT_LIBS
	optimized halley-winrt
	debug halley-winrt_d
	${HALLEY_PROJECT_LIBS}
	)
endif ()



set(HALLEY_PROJECT_INCLUDE_DIRS
	${HALLEY_PATH}/include
	${HALLEY_PATH}/src/contrib
	${HALLEY_PATH}/src/engine/core/include
	${HALLEY_PATH}/src/engine/net/include
	${HALLEY_PATH}/src/engine/utils/include
	${HALLEY_PATH}/src/engine/entity/include
	${HALLEY_PATH}/src/engine/audio/include
	${HALLEY_PATH}/src/engine/lua/include
	${HALLEY_PATH}/src/engine/ui/include
	${Boost_INCLUDE_DIR} 
	)
	
set(HALLEY_PROJECT_LIB_DIRS
	${HALLEY_PATH}/lib
	)



function(halleyProject name sources headers genDefinitions targetDir)
	set(EMBED ${HALLEY_PROJECT_EMBED})
	set(HALLEY_PROJECT_EMBED 0)

	if (EMBED)
		add_subdirectory(halley)
	endif()

	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${targetDir})
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${targetDir})
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${targetDir})
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${targetDir})

	file (GLOB_RECURSE ${name}_sources_gen "gen/*.cpp")
	file (GLOB_RECURSE ${name}_sources_systems "src/systems/*.cpp")
	file (GLOB_RECURSE ${name}_headers_gen "gen/*.h")
	
	set(proj_sources ${sources} ${${name}_sources_gen} ${${name}_sources_systems})
	set(proj_headers ${headers} ${${name}_headers_gen} ${genDefinitions})

	assign_source_group(${proj_sources})
	assign_source_group(${proj_headers})

	include_directories("." "gen/cpp" ${HALLEY_PROJECT_INCLUDE_DIRS})
	link_directories(${HALLEY_PROJECT_LIB_DIRS})

	if (HOTRELOAD)
		set(HALLEY_RUNNER_PATH ${HALLEY_PATH}\\bin\\halley-runner.exe)
		set(HALLEY_RUNNER_DEBUG_PATH ${HALLEY_PATH}\\bin\\halley-runner_d.exe)
		configure_file(${HALLEY_PATH}/cmake/halley_game.vcxproj.user.in ${CMAKE_CURRENT_BINARY_DIR}/${name}.vcxproj.user @ONLY) 
	endif()

	if (HOTRELOAD)
		add_library(${name} SHARED ${proj_sources} ${proj_headers})
		add_definitions(-DHALLEY_SHARED_LIBRARY)
	else()
		add_executable(${name} WIN32 ${proj_sources} ${proj_headers})
		add_definitions(-DHALLEY_EXECUTABLE)
	endif()

	if (MSVC)
		add_precompiled_header(${name} prec.h FORCEINCLUDE SOURCE_CXX prec.cpp)
		set_target_properties(${name} PROPERTIES LINK_FLAGS_RELEASE "/PDBSTRIPPED:\"${targetDir}/${name}_stripped.pdb\"")
	endif()

	if (EMSCRIPTEN)
		set_target_properties(${name} PROPERTIES SUFFIX ".bc")
	endif()

	if (EMBED)
		target_link_libraries(${name} halley-ui halley-core halley-entity halley-audio halley-net halley-lua halley-utils ${HALLEY_PROJECT_EXTERNAL_LIBS})
		if (USE_OPENGL)
			target_link_libraries(${name} halley-opengl)
		endif ()
		if (USE_SDL2)
			target_link_libraries(${name} halley-sdl)
		endif ()
		if (USE_ASIO)
			target_link_libraries(${name} halley-asio)
		endif ()
		if (USE_DX11)
			target_link_libraries(${name} halley-dx11)
		endif ()
		if (USE_WINRT)
			target_link_libraries(${name} halley-winrt)
		endif ()
		if (USE_MEDIA_FOUNDATION)
			target_link_libraries(${name} halley-mf)
		endif ()
	else ()
		target_link_libraries(${name} ${HALLEY_PROJECT_LIBS})
	endif ()
	
	if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "WindowsStore")
		set_target_properties(${name} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
	endif ()
endfunction(halleyProject)

function(halleyProjectCodegen name sources headers genDefinitions targetDir)
	add_custom_target(${name}-codegen ALL ${HALLEY_PATH}/bin/halley-cmd import ${targetDir}/.. ${HALLEY_PATH} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DEPENDS ${genDefinitions})
	halleyProject(${name} "${sources}" "${headers}" "${genDefinitions}" "${targetDir}")
	add_dependencies(${name} ${PROJECT_NAME}-codegen)

	if (TARGET halley-cmd)
		add_dependencies(${PROJECT_NAME}-codegen halley-cmd)
	endif ()
endfunction(halleyProjectCodegen)
