project (halley-ecs-standard)

include_directories(
    ${Boost_INCLUDE_DIR}
    ${HALLEY_PATH}/include
    ${HALLEY_PATH}/shared_gen/cpp
    "../../engine/utils/include"
     "../../engine/core/include"
)

file (GLOB_RECURSE HEADERS_GEN "../../../shared_gen/*.h")

set(SOURCES
    "src/systems/audio_system.cpp"
    "src/systems/network_lock_system.cpp"
    "src/systems/network_receive_system.cpp"
    "src/systems/network_send_system.cpp"
    "src/systems/particle_system.cpp"
    "src/systems/script_system.cpp"
    "src/systems/scriptable_query_system.cpp"
    "src/systems/sprite_animation_system.cpp"
)

set(HEADERS
    ${HEADERS_GEN}
)

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-ecs-standard ${SOURCES} ${HEADERS})
target_link_libraries(halley-ecs-standard halley-engine)
