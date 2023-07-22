project (halley-ecs-standard)

include_directories(
    ${Boost_INCLUDE_DIR}
    ${HALLEY_PATH}/include
    ${HALLEY_PATH}/shared_gen/cpp
    "../../engine/utils/include"
     "../../engine/core/include"
)

set(SOURCES
    "src/systems/script_system.cpp"
        )

set(HEADERS
    ""
        )

assign_source_group(${SOURCES})

add_library (halley-ecs-standard ${SOURCES} ${HEADERS})
target_link_libraries(halley-ecs-standard halley-engine)
