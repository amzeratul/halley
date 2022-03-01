project (halley-net)

include_directories(${Boost_INCLUDE_DIR} "include" "include/halley/net" "../core/include" "../utils/include" "../entity/include")

set(SOURCES
        "src/connection/ack_unreliable_connection.cpp"
        "src/connection/http.cpp"
        "src/connection/instability_simulator.cpp"
        "src/connection/message_queue.cpp"
        "src/connection/message_queue_tcp.cpp"
        "src/connection/message_queue_udp.cpp"
        "src/connection/network_packet.cpp"
        "src/connection/network_service.cpp"

        "src/entity/entity_network_message.cpp"
        "src/entity/entity_network_remote_peer.cpp"
        "src/entity/entity_network_session.cpp"

        "src/session/network_session_control_messages.cpp"
        "src/session/network_session.cpp"
        "src/session/shared_data.cpp"
        )

set(HEADERS
        "include/halley/net/halley_net.h"

        "include/halley/net/connection/ack_unreliable_connection.h"
        "include/halley/net/connection/http.h"
        "include/halley/net/connection/iconnection.h"
        "include/halley/net/connection/imessage_stream.h"
        "include/halley/net/connection/instability_simulator.h"
        "include/halley/net/connection/message_queue.h"
        "include/halley/net/connection/message_queue_tcp.h"
        "include/halley/net/connection/message_queue_udp.h"
        "include/halley/net/connection/network_message.h"
        "include/halley/net/connection/network_packet.h"
        "include/halley/net/connection/network_service.h"
        "include/halley/net/connection/standard_message_stream.h"

        "include/halley/net/entity/entity_network_message.h"
        "include/halley/net/entity/entity_network_remote_peer.h"
        "include/halley/net/entity/entity_network_session.h"

        "include/halley/net/session/network_session_control_messages.h"
        "include/halley/net/session/network_session_messages.h"
        "include/halley/net/session/network_session_peer.h"
        "include/halley/net/session/network_session.h"
        "include/halley/net/session/shared_data.h"
        )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-net ${SOURCES} ${HEADERS})
target_link_libraries(halley-net halley-utils halley-entity)
