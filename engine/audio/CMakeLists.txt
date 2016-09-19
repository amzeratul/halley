project (halley-audio)

include_directories(${Boost_INCLUDE_DIR} "include/halley/audio" "../utils/include" "../core/include")

set(SOURCES
        "src/audio_engine.cpp"
        "src/audio_facade.cpp"
        )

set(HEADERS
        "include/halley/audio/audio_buffer.h"
        "include/halley/audio/audio_engine.h"
        "include/halley/audio/audio_facade.h"
        )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-audio ${SOURCES} ${HEADERS})
