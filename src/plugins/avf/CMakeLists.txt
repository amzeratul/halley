project (halley-avf)

include_directories(${BOOST_INCLUDE_DIR} "../../engine/utils/include" "../../engine/core/include" "../../engine/audio/include")

set(SOURCES
	"src/avf_plugin.cpp"
	"src/avf_movie_api.mm"
	"src/avf_movie_player.mm"
	)

set(HEADERS
	"src/avf_movie_api.h"
	"src/avf_movie_player.h"
	)

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-avf ${SOURCES} ${HEADERS})
set_target_properties(halley-avf PROPERTIES DISABLE_PRECOMPILE_HEADERS ON)
target_link_libraries(halley-avf
	halley-engine
	"-framework AVFoundation"
	"-framework CoreMedia"
)
