project (halley-socketio)

include_directories("../../engine/utils/include" "../../engine/core/include" "../../engine/net/include")

set(SOURCES
    "src/socketio_network_api.cpp"
    "src/socketio_plugin.cpp"
    "src/socketio_network_service.cpp"
    )

set(HEADERS
    "src/socketio_network_api.h"
    "src/socketio_network_service.h"
    )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-socketio ${SOURCES} ${HEADERS})
target_link_libraries(halley-socketio halley-engine)