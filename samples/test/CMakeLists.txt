cmake_minimum_required (VERSION 3.0)

project (halley-sample-test)

set (${PROJECT_NAME}_sources
		"gen/cpp/registry.cpp"

		"src/main.cpp"
		"src/test_stage.cpp"
		"src/prec.cpp"

		"src/systems/movement_system.cpp"
		"src/systems/render_system.cpp"
		"src/systems/spawn_sprite_system.cpp"
		"src/systems/sprite_animation_system.cpp"
		"src/systems/time_system.cpp"
		)

set (${PROJECT_NAME}_headers
		"gen/cpp/components/position_component.h"
		"gen/cpp/components/sprite_animation_component.h"
		"gen/cpp/components/sprite_component.h"
		"gen/cpp/components/time_component.h"
		"gen/cpp/components/velocity_component.h"
		"gen/cpp/messages/expire_message"
		"gen/cpp/systems/movement_system.h"
		"gen/cpp/systems/render_system.h"
		"gen/cpp/systems/spawn_sprite_system.h"
		"gen/cpp/systems/sprite_animation_system.h"
		"gen/cpp/systems/time_system.h"
		)

add_precompiled_header(${PROJECT_NAME} prec.h FORCEINCLUDE)

# From http://stackoverflow.com/questions/31422680/how-to-set-visual-studio-filters-for-nested-sub-directory-using-cmake
function(assign_source_group)
	foreach(_source IN ITEMS ${ARGN})
		if (IS_ABSOLUTE "${_source}")
			file(RELATIVE_PATH _source_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_source}")
		else()
			set(_source_rel "${_source}")
		endif()
		get_filename_component(_source_path "${_source_rel}" PATH)
		string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
		source_group("${_source_path_msvc}" FILES "${_source}")
	endforeach()
endfunction(assign_source_group)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	set(EXTRA_LIBS)
else()
	set(EXTRA_LIBS pthread)
endif()

assign_source_group(${${PROJECT_NAME}_sources})
assign_source_group(${${PROJECT_NAME}_headers})

find_package(SDL2 REQUIRED)
find_package(YamlCpp REQUIRED)
find_package(Boost COMPONENTS system filesystem REQUIRED)

include_directories("gen/cpp" "../../include" "../../core/include" "../../entity/include" "../../utils/include" "../../opengl/include" ${YAMLCPP_INCLUDE_DIR})

# Run codegen
add_custom_target(codegen ALL ${CMAKE_HOME_DIRECTORY}/bin64/halley-cmd codegen gen_src gen WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies(codegen halley-cmd)

add_executable (${PROJECT_NAME} ${${PROJECT_NAME}_sources} ${${PROJECT_NAME}_headers})
target_link_libraries(${PROJECT_NAME}
		halley-core
		halley-utils
		halley-opengl
		halley-entity
		${SDL2_LIBRARIES}
		${YAMLCPP_LIBRARY}
		${Boost_FILESYSTEM_LIBRARY}
		${Boost_SYSTEM_LIBRARY}
		${EXTRA_LIBS}
		)

install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION ${CMAKE_HOME_DIRECTORY}/bin64
		LIBRARY DESTINATION ${CMAKE_HOME_DIRECTORY}/lib/x64
		ARCHIVE DESTINATION ${CMAKE_HOME_DIRECTORY}/lib/x64)
