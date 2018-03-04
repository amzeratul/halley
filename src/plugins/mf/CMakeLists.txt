project (halley-mf)

include_directories(${Boost_INCLUDE_DIR} "../../engine/utils/include" "../../engine/core/include")

set(SOURCES
    "src/mf_plugin.cpp"
    "src/mf_movie_api.cpp"
    "src/mf_movie_player.cpp"
    )

set(HEADERS
    "src/mf_movie_api.h"
    "src/mf_movie_player.h"
    )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-mf ${SOURCES} ${HEADERS})
target_link_libraries(halley-mf halley-core)