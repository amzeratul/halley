cmake_minimum_required (VERSION 3.0)

project (halley-editor)

set (editor_sources
	"prec.cpp"

	"src/main.cpp"
	"src/editor_root_stage.cpp"
	)

set (editor_headers
	"prec.h"
	"src/editor_root_stage.h"
	)

set (editor_gen_definitions
	"gen_src/editor.yaml"
	)

halleyProject(halley-editor "${editor_sources}" "${editor_headers}" "${editor_gen_definitions}" ${CMAKE_HOME_DIRECTORY}/bin)
