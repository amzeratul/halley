cmake_minimum_required (VERSION 2.8.10)

project (halley-sample-test)

set (${PROJECT_NAME}_headers "src/prec.h")
set (${PROJECT_NAME}_sources "src/prec.cpp" "src/main.cpp" "src/test_system.cpp")

add_executable (${PROJECT_NAME} ${${PROJECT_NAME}_headers}
								${${PROJECT_NAME}_sources})

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

assign_source_group(${${PROJECT_NAME}_sources})
assign_source_group(${${PROJECT_NAME}_headers})


#target_link_libraries(${PROJECT_NAME} halley-core)

install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib/static)
