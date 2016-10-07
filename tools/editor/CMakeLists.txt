cmake_minimum_required (VERSION 3.0)

project (halley-editor)

set (editor_sources
	"prec.cpp"

	"src/editor_root_stage.cpp"
	"src/halley_editor.cpp"
	"src/preferences.cpp"

	"src/console/console_window.cpp"

	"src/ui/taskbar/taskbar.cpp"
	)

set (editor_headers
	"prec.h"
	"src/editor_root_stage.h"
	"src/halley_editor.h"
	"src/preferences.h"

	"src/console/console_window.h"

	"src/ui/taskbar/taskbar.h"
	)

if (MSVC)
	set (editor_headers
		${EDITOR_HEADERS}
		"res/halley-editor.rc"
		"res/resource.h"
		"res/halley.ico"
	)
endif ()

set (editor_gen_definitions
	"gen_src/editor.yaml"
	)


if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(EXTRA_LIBS bz2 z)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(EXTRA_LIBS pthread)
endif()

include_directories(${FREETYPE_INCLUDE_DIR} "../tools/include")

halleyProjectCodegen(halley-editor "${editor_sources}" "${editor_headers}" "${editor_gen_definitions}" ${CMAKE_HOME_DIRECTORY}/bin)
add_dependencies(halley-editor halley-cmd halley-core halley-net halley-sdl halley-opengl)

target_link_libraries (halley-editor
        halley-tools
        ${FREETYPE_LIBRARIES}
        ${YAMLCPP_LIBRARY}
        ${EXTRA_LIBS}
        )

