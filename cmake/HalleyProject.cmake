cmake_minimum_required (VERSION 3.16)

SET(FASTLINK true CACHE BOOL "Enables /DEBUG:FASTLINK else /DEBUG:FULL")

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
set(CI_BUILD 0)

# Gitlab CI support
if (DEFINED ENV{CI_COMMIT_REF_SLUG})
	set(CI_BUILD 1)
	set(REF_SLUG $ENV{CI_COMMIT_REF_SLUG})
	string_starts_with(${REF_SLUG} "release" IS_RELEASE)
	if (IS_RELEASE)
		set(DEV_BUILD 0)
	endif()
endif()

if (DEV_BUILD EQUAL 1)
	add_definitions(-DDEV_BUILD)
endif()


# C++17 support
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -stdlib=libc++") # Apparently Clang on Mac needs this...
endif()
if (EMSCRIPTEN)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -stdlib=libc++")
	add_definitions(-s USE_SDL=2)
endif()


# Compiler-specific flags
if (MSVC)
	if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /fp:fast /WX /W3 -D_ENABLE_EXTENDED_ALIGNED_STORAGE /wd4275 /wd4251 /bigobj")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast /WX -D_ENABLE_EXTENDED_ALIGNED_STORAGE -Wno-unused-private-field -Wno-unused-variable -Wno-deprecated-declarations -Wno-microsoft-cast -Wno-switch -Wno-missing-declarations -Wno-string-plus-int -Wno-unused-function -Wno-assume")
	endif()

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17 /permissive-")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /sdl /Oi /Ot /Oy /Ob2 /Zi")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /sdl /Oi /Ot /Oy /Ob2 /Zi")

	if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
		if(CI_BUILD EQUAL 1)
			set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /GL")
			set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /LTCG /OPT:REF")
			set(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO} /LTCG /OPT:REF")
			set(CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO} /LTCG")
		endif()

		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GL")
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF")
		set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF")
		set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS_RELEASE} /LTCG")
	
		if (FASTLINK)
			set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG:FASTLINK")
		else()
			set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG:FULL")		
		endif()
	endif()
	set(CMAKE_CXX_STANDARD 17)

	if (${CMAKE_SYSTEM_NAME} MATCHES "WindowsStore")
		add_definitions(-D_WIN32_WINNT=0x0A00 -DWINVER=0x0A00 -DWINDOWS_STORE)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /await")
	elseif(REQUIRE_WINDOWS_10)
		add_definitions(-D_WIN32_WINNT=0x0A00 -DWINVER=0x0A00)
	else()
		add_definitions(-D_WIN32_WINNT=0x0601 -DWINVER=0x0601)
	endif()
	add_definitions(-D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
	add_definitions(-D_ENABLE_EXTENDED_ALIGNED_STORAGE)
else()
	if ((NOT EMSCRIPTEN) AND (NOT ANDROID_NDK))
		set(EXTRA_LIBS pthread)
	endif()
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")

	if (${CMAKE_CXX_COMPILER_ID} STREQUAL GNU) 
		if (HALLEY_ENABLE_STATIC_STDLIB)
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
		endif()

		# These are needed for DLLs built on GCC
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
	endif()
endif()


# GSL flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DGSL_THROW_ON_CONTRACT_VIOLATION")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DGSL_UNENFORCED_ON_CONTRACT_VIOLATION -DRELEASE_MODE")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DGSL_UNENFORCED_ON_CONTRACT_VIOLATION")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -DGSL_UNENFORCED_ON_CONTRACT_VIOLATION")


# Pick dependencies
set(USE_OPENGL 1)
set(USE_OPENGL_ES2 0)
set(USE_OPENGL_ES3 0)
set(USE_DX11 0)
set(USE_METAL 0)
set(USE_SDL2 1)
set(USE_ASIO 1)
set(USE_WINRT 0)
set(USE_XAUDIO2 0)
set(USE_MEDIA_FOUNDATION 0)
set(USE_AVFOUNDATION 0)
set(USE_ANDROID 0)
set(USE_HTTPLIB 1)

if (EMSCRIPTEN)
	set(USE_SDL2 0)
	set(USE_ASIO 0)
	set(USE_OPENGL 0)
	set(USE_OPENGL_ES2 1)
endif()

if (ANDROID_NDK)
	set(USE_SDL2 0)
	set(USE_ASIO 0)
	set(USE_OPENGL 0)
	set(USE_OPENGL_ES3 1)
	set(USE_ANDROID 1)
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set(USE_DX11 1)
	set(USE_MEDIA_FOUNDATION 1)
	if (REQUIRE_WINDOWS_10)
		set(USE_XAUDIO2 1)
	endif()
endif ()

if (${CMAKE_SYSTEM_NAME} MATCHES "WindowsStore")
	set(USE_DX11 1)
	set(USE_MEDIA_FOUNDATION 1)
	set(USE_SDL2 0)
	set(USE_OPENGL 0)
	set(USE_ASIO 0)
	set(USE_WINRT 1)
	set(USE_XAUDIO2 1)
endif ()

if (APPLE)
	set(USE_AVFOUNDATION 1)
	set(USE_METAL 0)
	set(USE_ASIO 1)
endif ()

# Libs
SET(CMAKE_INCLUDE_PATH "${HALLEY_PATH}/deps/include;${CMAKE_INCLUDE_PATH}")
SET(CMAKE_LIBRARY_PATH "${HALLEY_PATH}/deps/lib64;${CMAKE_LIBRARY_PATH}")
if (CMAKE_LIBRARY_PATH)
	link_directories("${CMAKE_LIBRARY_PATH}")
endif()

# SDL2
if (USE_SDL2)
	add_definitions(-DWITH_SDL2)
	if(EMSCRIPTEN)
		set(SDL2_INCLUDE_DIR "")
		set(SDL2_LIBRARIES "")
	elseif(NOT SDL2_INCLUDE_DIR OR NOT SDL2_LIBRARIES)
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
if (USE_OPENGL AND HALLEY_NO_OPENGL)
	set(USE_OPENGL 0)
endif ()
if (USE_OPENGL)
	add_definitions(-DWITH_OPENGL)
	find_package(OpenGL REQUIRED)
	if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
		find_package(X11 REQUIRED)
	else()
		set(X11_LIBRARIES "")
	endif ()
endif ()

# OpenGL ES
if (USE_OPENGL_ES2)
	add_definitions(-DWITH_OPENGL_ES2)
	set(OPENGL_LIBRARIES "GLESv2")
endif ()

if (USE_OPENGL_ES3)
	add_definitions(-DWITH_OPENGL_ES3)
	set(OPENGL_LIBRARIES "GLESv3")
endif ()

# Asio
if (USE_ASIO)
	add_definitions(-DWITH_ASIO)
endif ()

# DX11
if (USE_DX11)
	add_definitions(-DWITH_DX11)
endif()

# Metal
if (USE_METAL AND HALLEY_NO_METAL)
	set(USE_METAL 0)
endif ()
if (USE_METAL)
	add_definitions(-DWITH_METAL)
endif()

# WinRT
if (USE_WINRT)
	if (${CMAKE_SYSTEM_NAME} MATCHES "WindowsStore")
		add_definitions(-DWITH_WINRT)
	endif()
endif()

# XAudio2
if (USE_XAUDIO2)
	add_definitions(-DWITH_XAUDIO2)
endif()

# Android
if (USE_ANDROID)
	add_definitions(-DWITH_ANDROID)
endif()

# Microsoft Media Foundation
if (USE_MEDIA_FOUNDATION)
	add_definitions(-DWITH_MEDIA_FOUNDATION)
endif()

# cpp-httplib
if (USE_HTTPLIB)
	add_definitions(-DWITH_HTTPLIB)
endif()

# SSL
if (USE_SSL)
	set(OPENSSL_USE_STATIC_LIBS TRUE)
	set(OPENSSL_MSVC_STATIC_RT TRUE)
	find_Package(OpenSSL REQUIRED)
	add_definitions(-DWITH_SSL)
endif()

# Apple AVFoundation
if (USE_AVFOUNDATION)
  add_definitions(-DWITH_AVFOUNDATION)
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

	if (BUILD_MACOSX_BUNDLE)
		add_definitions(-DHALLEY_MACOSX_BUNDLE)
	endif()
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
	${OPENSSL_LIBRARIES}
	${X11_LIBRARIES}
	${EXTRA_LIBS}
	)

set(HALLEY_PROJECT_INCLUDE_DIRS
	${HALLEY_PATH}/include
	${HALLEY_PATH}/shared_gen/cpp
	${HALLEY_PATH}/src/contrib
	${HALLEY_PATH}/src/engine/core/include
	${HALLEY_PATH}/src/engine/lua/include
	${HALLEY_PATH}/src/engine/editor_extensions/include
	${Boost_INCLUDE_DIR} 
	${OPENSSL_INCLUDE_DIR}
	)

set(HALLEY_PROJECT_LIBS
	optimized halley-contrib
	optimized halley-engine
	optimized halley-ecs-standard
	debug halley-contrib_d
	debug halley-engine_d
	debug halley-ecs-standard_d
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

if (USE_AVFOUNDATION)
	set(HALLEY_PROJECT_LIBS
		optimized halley-avf
		debug halley-avf_d
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

if (USE_METAL)
set(HALLEY_PROJECT_LIBS
	optimized halley-metal
	debug halley-metal_d
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

if (USE_XAUDIO2)
set(HALLEY_PROJECT_LIBS
	optimized halley-xaudio2
	debug halley-xaudio2_d
	${HALLEY_PROJECT_LIBS}
	)
endif ()

if (USE_ANDROID)
set(HALLEY_PROJECT_LIBS
	optimized halley-android
	debug halley-android_d
	${HALLEY_PROJECT_LIBS}
	)
endif ()

if (USE_HTTPLIB)
set(HALLEY_PROJECT_LIBS
	optimized halley-httplib
	debug halley-httplib_d
	${HALLEY_PROJECT_LIBS}
	)
endif ()

set(HALLEY_PROJECT_LIB_DIRS
	${HALLEY_PATH}/lib
	)



function(halleyProject name sources headers proj_resources genDefinitions targetDir)
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
	assign_source_group(${proj_resources})

	include_directories("." "gen/cpp" ${HALLEY_PROJECT_INCLUDE_DIRS})
	link_directories(${HALLEY_PROJECT_LIB_DIRS})

	if (ANDROID_NDK)
		add_library(${name} SHARED ${proj_sources} ${proj_headers} ${proj_resources})
		add_definitions(-DHALLEY_SHARED_LIBRARY)
	elseif (HALLEY_MONOLITHIC)
		add_executable(${name} WIN32 ${proj_sources} ${proj_headers} ${proj_resources})
		target_compile_definitions(${name} PUBLIC HALLEY_EXECUTABLE)
	else()
		add_library(${name}-game STATIC ${proj_sources} ${proj_headers})
		add_library(${name}-dll SHARED ${HALLEY_PATH}/src/entry/halley_dll_entry.cpp)

		if (BUILD_MACOSX_BUNDLE)
			add_executable(${name} MACOSX_BUNDLE ${HALLEY_PATH}/src/entry/halley_exe_entry.cpp)
		else()
			add_executable(${name} WIN32 ${HALLEY_PATH}/src/entry/halley_exe_entry.cpp ${proj_resources})
		endif()
		#set_target_properties(${name}-exe PROPERTIES OUTPUT_NAME ${name})

		add_dependencies(${name}-dll ${name}-game)
		add_dependencies(${name} ${name}-game)
		
		target_compile_definitions(${name}-game PUBLIC HALLEY_STATIC_LIBRARY)
		target_compile_definitions(${name}-dll PUBLIC HALLEY_SHARED_LIBRARY)
		target_compile_definitions(${name} PUBLIC HALLEY_EXECUTABLE)

		# Setup default run paths for DLL
		# Note that I'm using release halley-cmd for debug, as currently they have the same name
		if (TARGET halley-cmd)
			set(HALLEY_RUNNER_PATH ${HALLEY_PATH}\\bin\\halley-cmd.exe)
			set(HALLEY_RUNNER_DEBUG_PATH ${HALLEY_PATH}\\bin\\halley-cmd.exe)
			configure_file(${HALLEY_PATH}/cmake/halley_game_dll.vcxproj.user.in ${CMAKE_CURRENT_BINARY_DIR}/${name}-dll.vcxproj.user @ONLY) 
			#add_dependencies(${name}-dll halley-cmd) # Having this allows you to run dll and make it build halley-cmd first, but it also causes problems building the game from editor
		endif()

		# Setup a gamebins target for external building
		add_custom_target(${name}-gamebins DEPENDS ${name} ${name}-dll)
	endif()

	if (HALLEY_MONOLITHIC)
		target_precompile_headers(${name} PRIVATE prec.h)
	else()
		target_precompile_headers(${name}-game PRIVATE prec.h)
	endif()

	if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
		set_target_properties(${name} PROPERTIES LINK_FLAGS_RELEASE "/PDBSTRIPPED:\"${targetDir}/${name}_stripped.pdb\"")
	endif()

	if (EMSCRIPTEN)
		set_target_properties(${name} PROPERTIES SUFFIX ".bc")
	endif()

	SET(LINK_LIBRARIES "")
	if (EMBED)
		SET(LINK_LIBRARIES ${LINK_LIBRARIES} halley-engine halley-ecs-standard ${HALLEY_PROJECT_EXTERNAL_LIBS})
		if (USE_OPENGL OR USE_OPENGL_ES2 OR USE_OPENGL_ES3)
			SET(LINK_LIBRARIES ${LINK_LIBRARIES} halley-opengl)
		endif ()
		if (USE_SDL2)
			SET(LINK_LIBRARIES ${LINK_LIBRARIES} halley-sdl)
		endif ()
		if (USE_ASIO)
			SET(LINK_LIBRARIES ${LINK_LIBRARIES} halley-asio)
		endif ()
		if (USE_DX11)
			SET(LINK_LIBRARIES ${LINK_LIBRARIES} halley-dx11)
		endif ()
		if (USE_WINRT)
			SET(LINK_LIBRARIES ${LINK_LIBRARIES} halley-winrt)
		endif ()
		if (USE_XAUDIO2)
			SET(LINK_LIBRARIES ${LINK_LIBRARIES} halley-xaudio2)
		endif ()
		if (USE_MEDIA_FOUNDATION)
			SET(LINK_LIBRARIES ${LINK_LIBRARIES} halley-mf)
		endif ()
		if (USE_AVFOUNDATION)
			SET(LINK_LIBRARIES ${LINK_LIBRARIES} halley-avf)
		endif ()
		if (USE_ANDROID)
			SET(LINK_LIBRARIES ${LINK_LIBRARIES} halley-android)
		endif ()
		if (USE_METAL)
			SET(LINK_LIBRARIES ${LINK_LIBRARIES} halley-metal)
		endif()
		if (USE_HTTPLIB)
			SET(LINK_LIBRARIES ${LINK_LIBRARIES} halley-httplib)
		endif()
	else ()
		SET(LINK_LIBRARIES ${LINK_LIBRARIES} ${HALLEY_PROJECT_LIBS})
	endif ()

	if (HALLEY_MONOLITHIC)
		target_link_libraries(${name} ${LINK_LIBRARIES})
	else ()
		target_link_libraries(${name} ${name}-game ${LINK_LIBRARIES})
		target_link_libraries(${name}-dll ${name}-game ${LINK_LIBRARIES})
	endif()

	set_property(TARGET ${name} PROPERTY VS_DPI_AWARE "PerMonitor")

	if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "WindowsStore")
		set_target_properties(${name} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
		if (TARGET ${name}-game)
			set_target_properties(${name}-game PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
		endif()
		if (TARGET ${name}-dll)
			set_target_properties(${name}-dll PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
		endif()
	endif ()

	if (BUILD_MACOSX_BUNDLE) # TODO check this out
	#	add_custom_command(TARGET ${name} POST_BUILD
	#		COMMAND "cp" "-R" "${CMAKE_CURRENT_SOURCE_DIR}/assets/" "$<TARGET_FILE_DIR:${name}>/../Resources"
	#	)
	endif ()
endfunction(halleyProject)

function(halleyProjectCodegen name sources headers project_resources genDefinitions targetDir)
	add_custom_target(${name}-codegen ALL ${HALLEY_PATH}/bin/halley-cmd import ${targetDir}/.. ${HALLEY_PATH} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} DEPENDS ${genDefinitions})
	halleyProject(${name} "${sources}" "${headers}" "${project_resources}" "${genDefinitions}" "${targetDir}")
	add_dependencies(${name} ${PROJECT_NAME}-codegen)

	if (TARGET halley-cmd)
		add_dependencies(${PROJECT_NAME}-codegen halley-cmd)
	endif ()
endfunction(halleyProjectCodegen)

function(halleyProjectBundleProperties name icon app_title copyright)
	if (BUILD_MACOSX_BUNDLE)
		get_filename_component(icon_name ${icon} NAME)

		set_target_properties(${name} PROPERTIES
			MACOSX_BUNDLE_BUNDLE_NAME "${app_title}"
			MACOSX_BUNDLE_COPYRIGHT "${copyright}"
			MACOSX_BUNDLE_GUI_IDENTIFIER "${app_title}"
			MACOSX_BUNDLE_ICON_FILE "${icon_name}"
		)

		add_custom_command(TARGET ${name} POST_BUILD
			COMMAND "cp" "${CMAKE_CURRENT_SOURCE_DIR}/${icon}" "$<TARGET_FILE_DIR:${name}>/../Resources/"
		)
	endif ()
endfunction(halleyProjectBundleProperties)
