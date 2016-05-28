project (halley-runner)

include_directories(${Boost_INCLUDE_DIR} "../core/include" "../utils/include")

set(SOURCES
		"src/dynamic_library.cpp"
		"src/dynamic_loader.cpp"
		"src/main.cpp"
		"src/windows_dll_runner.cpp"
		)
		
set(HEADERS
		"src/dynamic_library.h"
		"src/dynamic_loader.h"
		"src/win.h"
		"src/windows_dll_runner.h"
		)

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_executable (halley-runner ${SOURCES} ${HEADERS})
target_link_libraries (halley-runner
		halley-core
        halley-utils
        ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        ${EXTRA_LIBS}
        )

install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION ${CMAKE_HOME_DIRECTORY}/bin64
		LIBRARY DESTINATION ${CMAKE_HOME_DIRECTORY}/bin64
		ARCHIVE DESTINATION ${CMAKE_HOME_DIRECTORY}/bin64)
