project (halley-avf)

include_directories(${BOOST_INCLUDE_DIR} "../../engine/utils/include" "../../engine/core/include" "../../engine/audio/include")

set(SOURCES
	"src/avf_plugin.cpp"
	"src/avf_movie_api.cpp"
	)

set(HEADERS
	"src/avf_movie_api.h"
	)

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-avf ${SOURCES} ${HEADERS})
target_link_libraries(halley-avf halley-core halley-audio)
