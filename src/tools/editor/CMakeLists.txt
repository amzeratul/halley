cmake_minimum_required (VERSION 3.0)

project (halley-editor)

set (editor_sources
	"prec.cpp"

	"src/editor_root_stage.cpp"
	"src/halley_editor.cpp"
	"src/preferences.cpp"

	"src/assets/animation_editor.cpp"
	"src/assets/assets_editor_window.cpp"
	"src/assets/metadata_editor.cpp"

	"src/ui/console_window.cpp"
	"src/ui/editor_ui_factory.cpp"
	"src/ui/load_project_window.cpp"
	"src/ui/scroll_background.cpp"
	"src/ui/taskbar.cpp"
	"src/ui/toolbar.cpp"
	)

set (editor_headers
	"prec.h"
	"src/editor_root_stage.h"
	"src/halley_editor.h"
	"src/preferences.h"

	"src/assets/animation_editor.h"
	"src/assets/assets_editor_window.h"
	"src/assets/metadata_editor.h"

	"src/ui/console_window.h"
	"src/ui/editor_ui_factory.h"
	"src/ui/load_project_window.h"
	"src/ui/scroll_background.h"
	"src/ui/taskbar.h"
	"src/ui/toolbar.h"
	)

if (MSVC)
	set (editor_headers
		${editor_headers}
		"res/halley-editor.rc"
		"res/resource.h"
		"res/halley.ico"
	)
endif ()

set (editor_gen_definitions
	)


if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(EXTRA_LIBS bz2 z halley-metal)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	set(EXTRA_LIBS pthread)
endif()

include_directories(${FREETYPE_INCLUDE_DIR} "../tools/include")

halleyProjectCodegen(halley-editor "${editor_sources}" "${editor_headers}" "${editor_gen_definitions}" ${CMAKE_CURRENT_SOURCE_DIR}/../../../bin)
add_dependencies(halley-editor halley-cmd halley-core halley-net halley-sdl halley-opengl)
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	add_dependencies(halley-editor halley-metal)
endif()

target_link_libraries (halley-editor
        halley-tools
        ${FREETYPE_LIBRARIES}
        ${YAMLCPP_LIBRARY}
        ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        ${EXTRA_LIBS}
        )

