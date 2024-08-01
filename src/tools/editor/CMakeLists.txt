cmake_minimum_required (VERSION 3.16)

project (halley-editor)

SET(HALLEY_MONOLITHIC 1)

file(STRINGS "source_list.txt" editor_sources)
set_directory_properties(PROPERTIES CMAKE_CONFIGURE_DEPENDS "source_list.txt")
if (MSVC)
	file(STRINGS "resource_list_win32.txt" editor_resources)
	set_directory_properties(PROPERTIES CMAKE_CONFIGURE_DEPENDS "resource_list_win32.txt")
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND USE_METAL)
	set(EXTRA_LIBS bz2 z halley-metal)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	set(EXTRA_LIBS pthread)
endif()

include_directories(${FREETYPE_INCLUDE_DIR} "../tools/include")

halleyProjectCodegenV2(halley-editor "${editor_sources}" "${editor_resources}" ${CMAKE_CURRENT_SOURCE_DIR}/../../../bin)
add_dependencies(halley-editor halley-cmd halley-engine halley-sdl halley-opengl)
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND USE_METAL)
	add_dependencies(halley-editor halley-metal)
endif()

get_property(deps GLOBAL PROPERTY halley_external_plugin_dependencies)
if (NOT ("${deps}" STREQUAL ""))
    separate_arguments(deps)
    add_dependencies(halley-editor ${deps})
endif()

target_link_libraries (halley-editor
        halley-tools
        ${FREETYPE_LIBRARIES}
		${EXTRA_LIBS}
		${ShaderConductor_LIBRARY}
        )

add_custom_command(TARGET halley-editor POST_BUILD COMMAND ${HALLEY_PATH}/bin/halley-cmd write_version ${targetDir}/..)
