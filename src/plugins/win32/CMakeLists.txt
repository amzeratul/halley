project (halley-win32)

include_directories(${Boost_INCLUDE_DIR} "../../engine/utils/include" "../../engine/core/include")

set(SOURCES
    "src/win32_data_reader.cpp"
    "src/win32_plugin.cpp"
    "src/win32_system.cpp"
)

set(HEADERS
    "src/win32_data_reader.h"
    "src/win32_system.h"
)

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-win32 ${SOURCES} ${HEADERS})
target_link_libraries(halley-win32 halley-core)