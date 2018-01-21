project (halley-winrt)

include_directories(${Boost_INCLUDE_DIR} "../../engine/utils/include" "../../engine/core/include")

set(SOURCES
    "src/winrt_plugin.cpp"
    "src/winrt_system.cpp"
    )

set(HEADERS
    "src/winrt_system.h"
    )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-winrt ${SOURCES} ${HEADERS})
target_link_libraries(halley-winrt halley-core)