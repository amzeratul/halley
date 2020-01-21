project (halley-audio)

include_directories(${Boost_INCLUDE_DIR} "include/halley/audio" "../utils/include" "../core/include" "../../contrib/libogg/include" "../../contrib/libogg/lib" "../../contrib/libvorbis/include" "../../contrib/libvorbis/lib")

set(SOURCES
        "src/audio_buffer.cpp"
        "src/audio_clip.cpp"
        "src/audio_emitter.cpp"
        "src/audio_emitter_behaviour.cpp"
        "src/audio_engine.cpp"
        "src/audio_event.cpp"
        "src/audio_facade.cpp"
        "src/audio_filter_biquad.cpp"
        "src/audio_filter_resample.cpp"
        "src/audio_handle_impl.cpp"
        "src/audio_mixer.cpp"
        "src/audio_mixer_avx.cpp"
        "src/audio_mixer_sse.cpp"
        "src/audio_position.cpp"
        "src/audio_source_clip.cpp"
        "src/vorbis_dec.cpp"
        )

set(HEADERS
        "include/halley/audio/audio_clip.h"
        "include/halley/audio/audio_emitter_behaviour.h"
        "include/halley/audio/audio_event.h"
        "include/halley/audio/audio_facade.h"
        "include/halley/audio/audio_filter_biquad.h"
        "include/halley/audio/audio_position.h"
        "include/halley/audio/audio_source.h"
        "include/halley/audio/halley_audio.h"
        "include/halley/audio/vorbis_dec.h"
        "src/audio_buffer.h"
        "src/audio_emitter.h"
        "src/audio_engine.h"
        "src/audio_filter_resample.h"
        "src/audio_handle_impl.h"
        "src/audio_mixer.h"
        "src/audio_mixer_avx.h"
        "src/audio_mixer_sse.h"
        "src/audio_source_clip.h"
        )

set(SOURCES ${SOURCES} ${OGG_FILES} ${VORBIS_FILES})

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

if (MSVC)
        set_source_files_properties(src/audio_mixer_avx.cpp PROPERTIES COMPILE_FLAGS /arch:AVX)
else ()
        set_source_files_properties(src/audio_mixer_avx.cpp PROPERTIES COMPILE_FLAGS -mavx)
endif ()

add_library (halley-audio ${SOURCES} ${HEADERS})
target_link_libraries (halley-audio halley-contrib)
