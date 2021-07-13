cmake_minimum_required (VERSION 3.10)

project (halley-launcher)

SET(HALLEY_MONOLITHIC 1)

set (launcher_sources
	"prec.cpp"

	"src/choose_project.cpp"
	"src/launcher.cpp"
	"src/launcher_save_data.cpp"
	"src/launcher_stage.cpp"
	)

set (launcher_headers
	"prec.h"

	"src/choose_project.h"
	"src/launcher.h"
	"src/launcher_save_data.h"
	"src/launcher_stage.h"
	)

if (MSVC)
	set (launcher_headers
		${launcher_headers}
		"res/halley-launcher.rc"
		"res/resource.h"
		"res/halley.ico"
	)
endif ()

set (launcher_gen_definitions
	)


if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(EXTRA_LIBS bz2 z halley-metal)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	set(EXTRA_LIBS pthread)
endif()

include_directories(${FREETYPE_INCLUDE_DIR} "../tools/include")

halleyProjectCodegen(halley-launcher "${launcher_sources}" "${launcher_headers}" "" "${launcher_gen_definitions}" ${CMAKE_CURRENT_SOURCE_DIR}/bin)
add_dependencies(halley-launcher halley-cmd halley-core halley-net halley-sdl halley-opengl)
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	add_dependencies(halley-launcher halley-metal)
endif()

get_property(deps GLOBAL PROPERTY halley_external_plugin_dependencies)
if (NOT ("${deps}" STREQUAL ""))
    separate_arguments(deps)
    add_dependencies(halley-launcher ${deps})
endif()

target_link_libraries (halley-launcher
        halley-tools
        ${FREETYPE_LIBRARIES}
        ${YAMLCPP_LIBRARY}
        ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
		${EXTRA_LIBS}
		${ShaderConductor_LIBRARY}
        )
