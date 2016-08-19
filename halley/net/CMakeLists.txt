project (halley-net)

include_directories("include/halley/net" "../utils/include")

set(SOURCES
        "src/http.cpp"
        "src/instability_simulator.cpp"
        "src/message_queue.cpp"
        "src/network_message.cpp"
        "src/network_packet.cpp"
        "src/network_service.cpp"
        "src/reliable_connection.cpp"
        "src/standard_message_stream.cpp"
        "src/udp_connection.cpp"
        )

set(HEADERS
        "include/halley/net/halley_net.h"
        "include/halley/net/http.h"
        "include/halley/net/iconnection.h"
        "include/halley/net/imessage_stream.h"
        "include/halley/net/instability_simulator.h"
        "include/halley/net/message_queue.h"
        "include/halley/net/network_message.h"
        "include/halley/net/network_packet.h"
        "include/halley/net/network_service.h"
        "include/halley/net/reliable_connection.h"
        "include/halley/net/standard_message_stream.h"
        "src/udp_connection.h"
        )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-net ${SOURCES} ${HEADERS})
target_link_libraries(halley-net halley-utils)
