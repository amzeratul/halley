cmake_minimum_required (VERSION 3.0)

project (halley-editor)

set (editor_sources
	"prec.cpp"

	"src/editor_root_stage.cpp"
	"src/halley_editor.cpp"
	"src/preferences.cpp"

	"src/console/console_window.cpp"

	"src/project/project.cpp"

	"src/tasks/editor_task.cpp"
	"src/tasks/editor_task_set.cpp"
	"src/tasks/assets/check_assets_task.cpp"
	"src/tasks/assets/delete_assets_task.cpp"
	"src/tasks/assets/import_assets_task.cpp"
	"src/tasks/assets/import_assets_database.cpp"

	"src/ui/taskbar/taskbar.cpp"
	)

set (editor_headers
	"prec.h"
	"src/editor_root_stage.h"
	"src/halley_editor.h"
	"src/preferences.h"

	"src/console/console_window.h"

	"src/project/project.h"

	"src/tasks/editor_task.h"
	"src/tasks/editor_task_set.h"
	"src/tasks/assets/check_assets_task.h"
	"src/tasks/assets/delete_assets_task.h"
	"src/tasks/assets/import_assets_task.h"
	"src/tasks/assets/import_assets_database.h"

	"src/ui/taskbar/taskbar.h"
	)

set (editor_gen_definitions
	"gen_src/editor.yaml"
	)

halleyProject(halley-editor "${editor_sources}" "${editor_headers}" "${editor_gen_definitions}" ${CMAKE_HOME_DIRECTORY}/bin)
