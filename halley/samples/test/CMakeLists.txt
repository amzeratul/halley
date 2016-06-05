cmake_minimum_required (VERSION 3.0)

project (halley-sample-test)

set (test_sources
		"prec.cpp"

		"src/main.cpp"
		"src/test_stage.cpp"

		"src/systems/movement_system.cpp"
		"src/systems/render_system.cpp"
		"src/systems/spawn_sprite_system.cpp"
		"src/systems/sprite_animation_system.cpp"
		"src/systems/time_system.cpp"
		)

set (test_headers
		"prec.h"
		"src/test_stage.h"
		)

halleyProject(halley-sample-test "${test_sources}" "${test_headers}" ${CMAKE_HOME_DIRECTORY}/bin)
