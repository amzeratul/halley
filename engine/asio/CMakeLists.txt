project (halley-asio)

include_directories(${Boost_INCLUDE_DIR} ${SDL2_INCLUDE_DIR} "include/halley/asio" "../utils/include" "../core/include" "../net/include")

set(SOURCES
    "src/asio_network_api.cpp"
    "src/asio_network_service.cpp"
    "src/asio_plugin.cpp"
    "src/asio_udp_connection.cpp"
    )

set(HEADERS
    "src/asio_network_api.h"
    "src/asio_network_service.h"
    "src/asio_udp_connection.h"
    )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-asio ${SOURCES} ${HEADERS})
target_link_libraries(halley-asio halley-core halley-net)