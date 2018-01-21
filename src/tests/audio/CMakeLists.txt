cmake_minimum_required (VERSION 3.0)

project (halley-test-audio)

set (audio_test_sources
	"prec.cpp"

	"src/main.cpp"
	"src/test_stage.cpp"
	)

set (audio_test_headers
	"prec.h"
	"src/test_stage.h"
	)

set (audio_test_gen_definitions
	)

halleyProjectCodegen(halley-test-audio "${audio_test_sources}" "${audio_test_headers}" "${audio_test_gen_definitions}" ${CMAKE_CURRENT_SOURCE_DIR}/bin)
