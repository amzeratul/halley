project (halley-xaudio2)

include_directories(${Boost_INCLUDE_DIR} "../../engine/utils/include" "../../engine/core/include")

set(SOURCES
    "src/xaudio2_audio_output.cpp"
    "src/xaudio2_plugin.cpp"
    )

set(HEADERS

    "src/xaudio2_audio_output.h"
    )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-xaudio2 ${SOURCES} ${HEADERS})
target_link_libraries(halley-xaudio2 halley-core)