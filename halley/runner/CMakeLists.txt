project (halley-runner)

include_directories(${Boost_INCLUDE_DIR} ${SDL2_INCLUDE_DIR}  "../core/include" "../utils/include")

set(SOURCES
		"src/dynamic_library.cpp"
		"src/dynamic_loader.cpp"
		"src/memory_patcher.cpp"
		"src/main.cpp"
		"src/symbol_loader.cpp"
		)
		
set(HEADERS
		"src/dynamic_library.h"
		"src/dynamic_loader.h"
		"src/memory_patcher.h"
		"src/symbol_loader.h"
		"src/win.h"
		)

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_executable (halley-runner ${SOURCES} ${HEADERS})
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

target_link_libraries (halley-runner
		halley-utils
		${SDL2_LIBRARIES}
        ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
		${Boost_THREAD_LIBRARY}
        ${EXTRA_LIBS}
        )

install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION ${CMAKE_HOME_DIRECTORY}/bin
		LIBRARY DESTINATION ${CMAKE_HOME_DIRECTORY}/bin
		ARCHIVE DESTINATION ${CMAKE_HOME_DIRECTORY}/bin)
