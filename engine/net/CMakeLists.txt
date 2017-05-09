project (halley-net)

include_directories(${Boost_INCLUDE_DIR} "include/halley/net" "../utils/include")

set(SOURCES
        "src/connection/http.cpp"
        "src/connection/instability_simulator.cpp"
        "src/connection/message_queue.cpp"
        "src/connection/network_packet.cpp"
        "src/connection/reliable_connection.cpp"

        "src/session/network_session.cpp"
        )

set(HEADERS
        "include/halley/net/halley_net.h"

        "include/halley/net/connection/http.h"
        "include/halley/net/connection/iconnection.h"
        "include/halley/net/connection/imessage_stream.h"
        "include/halley/net/connection/instability_simulator.h"
        "include/halley/net/connection/message_queue.h"
        "include/halley/net/connection/network_message.h"
        "include/halley/net/connection/network_packet.h"
        "include/halley/net/connection/network_service.h"
        "include/halley/net/connection/reliable_connection.h"
        "include/halley/net/connection/standard_message_stream.h"

        "include/halley/net/session/network_session_control_messages.h"
        "include/halley/net/session/network_session_messages.h"
        "include/halley/net/session/network_session_peer.h"
        "include/halley/net/session/network_session.h"
        )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-net ${SOURCES} ${HEADERS})
target_link_libraries(halley-net halley-utils)
