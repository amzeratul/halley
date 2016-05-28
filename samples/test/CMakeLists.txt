cmake_minimum_required (VERSION 3.0)

project (halley-sample-test)

set (${PROJECT_NAME}_sources
		"prec.cpp"

		"gen/cpp/registry.cpp"

		"src/main.cpp"
		"src/test_stage.cpp"

		"src/systems/movement_system.cpp"
		"src/systems/render_system.cpp"
		"src/systems/spawn_sprite_system.cpp"
		"src/systems/sprite_animation_system.cpp"
		"src/systems/time_system.cpp"
		)

set (${PROJECT_NAME}_headers
		"prec.h"
		"src/test_stage.h"
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

assign_source_group(${${PROJECT_NAME}_sources})
assign_source_group(${${PROJECT_NAME}_headers})

include_directories("." "gen/cpp" "../../include" "../../core/include" "../../entity/include" "../../utils/include" "../../opengl/include" ${YAMLCPP_INCLUDE_DIR})

# Run codegen
add_custom_target(codegen ALL ${CMAKE_HOME_DIRECTORY}/bin64/halley-cmd codegen gen_src gen WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies(codegen halley-cmd)
set(LIBS
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

if(MSVC)
	add_library(${PROJECT_NAME} SHARED ${${PROJECT_NAME}_sources} ${${PROJECT_NAME}_headers})
	add_definitions(-DHALLEY_SHARED_LIBRARY)
else()
	add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_sources} ${${PROJECT_NAME}_headers})
	add_definitions(-DHALLEY_EXECUTABLE)
endif()
target_link_libraries(${PROJECT_NAME} ${LIBS})

if(MSVC)
	add_precompiled_header(${PROJECT_NAME} prec.h FORCEINCLUDE SOURCE_CXX prec.cpp)
endif()

install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION ${CMAKE_HOME_DIRECTORY}/bin64
		LIBRARY DESTINATION ${CMAKE_HOME_DIRECTORY}/bin64
		ARCHIVE DESTINATION ${CMAKE_HOME_DIRECTORY}/bin64)
