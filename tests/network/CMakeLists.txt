cmake_minimum_required (VERSION 3.0)

project (halley-network-test)

set (network_test_sources
	"prec.cpp"

	"src/main.cpp"
	"src/test_stage.cpp"
	)

set (network_test_headers
	"prec.h"
	"src/test_stage.h"
	)

set (network_test_gen_definitions
	)

halleyProjectCodegen(halley-network-test "${network_test_sources}" "${network_test_headers}" "${network_test_gen_definitions}" ${CMAKE_CURRENT_SOURCE_DIR}/bin)
add_dependencies(halley-network-test halley-cmd)
