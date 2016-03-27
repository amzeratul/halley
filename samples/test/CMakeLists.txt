cmake_minimum_required (VERSION 2.6)

project (halley-sample-test)

set (${PROJECT_NAME}_headers "src/prec.h")
set (${PROJECT_NAME}_sources "src/prec.cpp" "src/main.cpp" "src/test_system.cpp")

add_executable (${PROJECT_NAME} ${${PROJECT_NAME}_headers}
								${${PROJECT_NAME}_sources})

#target_link_libraries(${PROJECT_NAME} halley-core)

install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib/static)
