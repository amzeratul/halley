cmake_minimum_required (VERSION 3.0)

project (halley-test-entity)

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

halleyProjectCodegen(halley-test-entity "${entity_test_sources}" "${entity_test_headers}" "${entity_test_gen_definitions}" ${CMAKE_CURRENT_SOURCE_DIR}/bin)
