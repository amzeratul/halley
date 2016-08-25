cmake_minimum_required (VERSION 3.0)

project (halley-editor)

set (editor_sources
	"prec.cpp"

	"src/editor_root_stage.cpp"
	"src/main.cpp"

	"src/console/console_window.cpp"

	"src/project/project.cpp"

	"src/tasks/editor_task.cpp"
	)

set (editor_headers
	"prec.h"
	"src/editor_root_stage.h"

	"src/console/console_window.h"

	"src/project/project.h"

	"src/tasks/editor_task.h"
	)

set (editor_gen_definitions
	"gen_src/editor.yaml"
	)

halleyProject(halley-editor "${editor_sources}" "${editor_headers}" "${editor_gen_definitions}" ${CMAKE_HOME_DIRECTORY}/bin)
