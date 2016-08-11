cmake_minimum_required (VERSION 3.0)

project (halley-entity-test)

set (entity_test_sources
	"prec.cpp"

	"src/main.cpp"
	"src/test_stage.cpp"
	)

set (entity_test_headers
	"prec.h"
	"src/test_stage.h"
	)

set (entity_test_gen_definitions
	"gen_src/test.yaml"
	)

halleyProject(halley-entity-test "${entity_test_sources}" "${entity_test_headers}" "${entity_test_gen_definitions}" ${CMAKE_HOME_DIRECTORY}/bin)
