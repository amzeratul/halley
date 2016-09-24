project (halley-audio)

include_directories(${Boost_INCLUDE_DIR} "include/halley/audio" "../utils/include" "../core/include")

set(SOURCES
        "src/audio_clip.cpp"
        "src/audio_engine.cpp"
        "src/audio_facade.cpp"
        "src/audio_source.cpp"
        "src/vorbis_dec.cpp"
        )

set(HEADERS
        "include/halley/audio/audio_buffer.h"
        "include/halley/audio/audio_clip.h"
        "include/halley/audio/audio_engine.h"
        "include/halley/audio/audio_facade.h"
        "include/halley/audio/audio_source.h"
        "include/halley/audio/halley_audio.h"
        "src/vorbis_dec.h"
        )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-audio ${SOURCES} ${HEADERS})
