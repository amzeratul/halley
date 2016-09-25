project (halley-audio)

include_directories(${Boost_INCLUDE_DIR} ${OGG_INCLUDE_DIR} ${VORBIS_INCLUDE_DIR} "include/halley/audio" "../utils/include" "../core/include")

set(SOURCES
        "src/audio_clip.cpp"
        "src/audio_engine.cpp"
        "src/audio_facade.cpp"
        "src/audio_handle_impl.cpp"
        "src/audio_mixer.cpp"
        "src/audio_mixer_avx.cpp"
        "src/audio_mixer_sse.cpp"
        "src/audio_source.cpp"
        "src/audio_source_position.cpp"
        "src/vorbis_dec.cpp"
        )

set(HEADERS
        "include/halley/audio/audio_buffer.h"
        "include/halley/audio/audio_clip.h"
        "include/halley/audio/audio_engine.h"
        "include/halley/audio/audio_facade.h"
        "include/halley/audio/audio_mixer.h"
        "include/halley/audio/audio_source.h"
        "include/halley/audio/audio_source_position.h"
        "include/halley/audio/halley_audio.h"
        "src/vorbis_dec.h"
        "src/audio_handle_impl.h"
        "src/audio_mixer_avx.h"
        "src/audio_mixer_sse.h"
        )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

if (MSVC)
        set_source_files_properties(src/audio_mixer_avx.cpp PROPERTIES COMPILE_FLAGS /arch:AVX)
else ()
        set_source_files_properties(src/audio_mixer_avx.cpp PROPERTIES COMPILE_FLAGS -mavx)
endif ()

add_library (halley-audio ${SOURCES} ${HEADERS})
target_link_libraries (halley-audio ${OGG_LIBRARY} ${VORBIS_LIBRARY} ${VORBISFILE_LIBRARY})