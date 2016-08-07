project (halley-net)

include_directories("include/halley/net" "../utils/include")

set(SOURCES
        "src/http.cpp"
        "src/network.cpp"
        "src/udp_connection.cpp"
        "src/udp_connection_channel.cpp"
        )

set(HEADERS
        "include/halley/net/http.h"
        "include/halley/net/network.h"
        "include/halley/net/udp_connection.h"
        "include/halley/net/udp_connection_channel.h"
        )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-net ${SOURCES} ${HEADERS})
target_link_libraries(halley-net halley-utils)
