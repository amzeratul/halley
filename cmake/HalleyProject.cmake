include(PrecompiledHeader)

# C++14 support
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -stdlib=libc++") # Apparently Clang on Mac needs this...
endif()


# Compiler-specific flags
if (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /fp:fast")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GL /sdl /Oi /Ot /Oy")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
	set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS_RELEASE} /LTCG")
	set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG")
else()
	set(EXTRA_LIBS pthread)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")

	if (HALLEY_ENABLE_STATIC_STDLIB)
		if (CMAKE_CXX_COMPILER_ID EQUAL GNU) 
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


# Libs
if (CMAKE_LIBRARY_PATH)
	link_directories(CMAKE_INCLUDE_PATH)
endif()

# SDL2
set (SDL2_BUILDING_LIBRARY 1)
find_Package(SDL2 REQUIRED)

# GL
find_package(OpenGL REQUIRED)

# Boost
find_package(Boost REQUIRED)
add_definitions(-DBOOST_ALL_NO_LIB)

# Apple frameworks
if (APPLE)
	find_library(CARBON_LIBRARY Carbon)
	find_library(COCOA_LIBRARY Cocoa)
	find_library(COREAUDIO_LIBRARY CoreAudio)
	find_library(AUDIOUNIT_LIBRARY AudioUnit)
	find_library(FORCEFEEDBACK_LIBRARY ForceFeedback)
	find_library(IOKIT_LIBRARY IOKit)
	find_library(COREVIDEO_LIBRARY CoreVideo)

	mark_as_advanced(CARBON_LIBRARY COCOA_LIBRARY COREAUDIO_LIBRARY AUDIOUNIT_LIBRARY FORCEFEEDBACK_LIBRARY IOKIT_LIBRARY COREVIDEO_LIBRARY)

	set(EXTRA_LIBS ${EXTRA_LIBS} ${CARBON_LIBRARY} ${COCOA_LIBRARY} ${COREAUDIO_LIBRARY} ${AUDIOUNIT_LIBRARY} ${FORCEFEEDBACK_LIBRARY} ${IOKIT_LIBRARY} ${COREVIDEO_LIBRARY})
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

set(CMAKE_DEBUG_POSTFIX "_d")

set(HALLEY_PROJECT_LIBS
	optimized halley-core
	optimized halley-audio
	optimized halley-utils
	optimized halley-net
	optimized halley-opengl
	optimized halley-sdl
	optimized halley-entity
	debug halley-core_d
	debug halley-audio_d
	debug halley-utils_d
	debug halley-net_d
	debug halley-opengl_d
	debug halley-sdl_d
	debug halley-entity_d
	${SDL2_LIBRARIES}
	${EXTRA_LIBS}
	)

set(HALLEY_PROJECT_INCLUDE_DIRS
	${HALLEY_PATH}/include
	${HALLEY_PATH}/contrib
	${HALLEY_PATH}/engine/core/include
	${HALLEY_PATH}/engine/net/include
	${HALLEY_PATH}/engine/utils/include
	${HALLEY_PATH}/engine/entity/include
	${HALLEY_PATH}/engine/audio/include
	${Boost_INCLUDE_DIR} 
	)
	
set(HALLEY_PROJECT_LIB_DIRS
	${HALLEY_PATH}/lib
	)

function(halleyProject name sources headers genDefinitions targetDir)
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
		add_library(${name} SHARED ${proj_sources} ${proj_headers})
		add_definitions(-DHALLEY_SHARED_LIBRARY)
	else()
		add_executable(${name} WIN32 ${proj_sources} ${proj_headers})
		add_definitions(-DHALLEY_EXECUTABLE)
	endif()

	target_link_libraries(${name} ${HALLEY_PROJECT_LIBS})
	set_target_properties(${name} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

	if (HOTRELOAD)
		set(HALLEY_RUNNER_PATH ${HALLEY_PATH}\\bin\\halley-runner.exe)
		set(HALLEY_RUNNER_DEBUG_PATH ${HALLEY_PATH}\\bin\\halley-runner_d.exe)
		configure_file(${HALLEY_PATH}/cmake/halley_game.vcxproj.user.in ${CMAKE_CURRENT_BINARY_DIR}/${name}.vcxproj.user @ONLY) 
	endif()
	
	if(MSVC)
		add_precompiled_header(${name} prec.h FORCEINCLUDE SOURCE_CXX prec.cpp)
		add_definitions(-D_WIN32_WINNT=0x0600)
	endif()
endfunction(halleyProject)

function(halleyProjectCodegen name sources headers genDefinitions targetDir)
	add_custom_target(${name}-codegen ALL ${HALLEY_PATH}/bin/halley-cmd import ${targetDir}/.. ${HALLEY_PATH} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DEPENDS ${genDefinitions})
	halleyProject(${name} "${sources}" "${headers}" "${genDefinitions}" "${targetDir}")
	add_dependencies(${name} ${PROJECT_NAME}-codegen)

	if (TARGET halley-cmd)
		add_dependencies(${PROJECT_NAME}-codegen halley-cmd)
	endif ()
endfunction(halleyProjectCodegen)
