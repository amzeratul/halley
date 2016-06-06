cmake_minimum_required (VERSION 3.0)

project (halley-sample-test)

set (test_sources
	"prec.cpp"

	"src/main.cpp"
	"src/test_stage.cpp"
	)

set (test_headers
	"prec.h"
	"src/test_stage.h"
	)

set (test_gen_definitions
	"gen_src/test.yaml"
	)

halleyProject(halley-sample-test "${test_sources}" "${test_headers}" "${test_gen_definitions}" ${CMAKE_HOME_DIRECTORY}/bin)
