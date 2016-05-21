cmake_minimum_required (VERSION 2.8.12)

project (halley-sample-test)

set (${PROJECT_NAME}_sources
		"gen/cpp/registry.cpp"

		"src/main.cpp"
		"src/test_stage.cpp"

		"src/systems/movement_system.cpp"
		"src/systems/render_system.cpp"
		"src/systems/spawn_sprite_system.cpp"
		"src/systems/sprite_animation_system.cpp"
		"src/systems/time_system.cpp"
		)

include_directories("gen/cpp" "../../include" "../../core/include" "../../entity/include" "../../utils/include" "../../opengl/include")

#add_precompiled_header(${PROJECT_NAME} src/prec.h FORCEINCLUDE)

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

#assign_source_group(${${PROJECT_NAME}_sources})
#assign_source_group(${${PROJECT_NAME}_headers})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake_modules/")
find_Package(SDL2 REQUIRED)

add_custom_target(codegen ALL ${CMAKE_HOME_DIRECTORY}/bin64/halley-cmd codegen gen_src gen WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_executable (${PROJECT_NAME} ${${PROJECT_NAME}_sources})
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES} halley-core halley-utils halley-opengl halley-entity yaml-cpp stdc++fs pthread)

install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION ${CMAKE_HOME_DIRECTORY}/bin64
		LIBRARY DESTINATION ${CMAKE_HOME_DIRECTORY}/lib/x64
		ARCHIVE DESTINATION ${CMAKE_HOME_DIRECTORY}/lib/x64)
