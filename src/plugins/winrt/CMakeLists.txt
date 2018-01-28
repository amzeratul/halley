project (halley-winrt)

include_directories(${Boost_INCLUDE_DIR} "../../engine/utils/include" "../../engine/core/include")

set(SOURCES
    "src/winrt_gamepad.cpp"
    "src/winrt_input.cpp"
    "src/winrt_plugin.cpp"
    "src/winrt_system.cpp"
    "src/xaudio2_audio_output.cpp"
    )

set(HEADERS
    "src/winrt_gamepad.h"
    "src/winrt_input.h"
    "src/winrt_system.h"
    "src/xaudio2_audio_output.h"
    )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-winrt ${SOURCES} ${HEADERS})
target_link_libraries(halley-winrt halley-core)