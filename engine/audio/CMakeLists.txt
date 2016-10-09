project (halley-audio)

include_directories(${Boost_INCLUDE_DIR} "include/halley/audio" "../utils/include" "../core/include" "../../contrib/libogg/include" "../../contrib/libogg/lib" "../../contrib/libvorbis/include" "../../contrib/libvorbis/lib")

set(SOURCES
        "src/audio_clip.cpp"
        "src/audio_engine.cpp"
        "src/audio_facade.cpp"
        "src/audio_handle_impl.cpp"
        "src/audio_mixer.cpp"
        "src/audio_mixer_avx.cpp"
        "src/audio_mixer_sse.cpp"
        "src/audio_source.cpp"
        "src/audio_source_behaviour.cpp"
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
        "include/halley/audio/audio_source_behaviour.h"
        "include/halley/audio/audio_source_position.h"
        "include/halley/audio/halley_audio.h"
        "include/halley/audio/vorbis_dec.h"
        "src/audio_handle_impl.h"
        "src/audio_mixer_avx.h"
        "src/audio_mixer_sse.h"
        )

file (GLOB_RECURSE OGG_FILES "../../contrib/libogg/*.c")
file (GLOB_RECURSE VORBIS_FILES "../../contrib/libvorbis/*.c")

if(MSVC)
        add_definitions("-wd4996 -wd4244 -wd4100 -wd4267 -wd4189 -wd4305 -wd4127 -wd4703 -wd4706")
        add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

set(SOURCES ${SOURCES} ${OGG_FILES} ${VORBIS_FILES})

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

if (MSVC)
        set_source_files_properties(src/audio_mixer_avx.cpp PROPERTIES COMPILE_FLAGS /arch:AVX)
else ()
        set_source_files_properties(src/audio_mixer_avx.cpp PROPERTIES COMPILE_FLAGS -mavx)
endif ()

add_library (halley-audio ${SOURCES} ${HEADERS})
