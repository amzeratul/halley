project (halley-sdl3)

include_directories(${Boost_INCLUDE_DIR} "../../engine/utils/include" "../../engine/core/include")

set(SOURCES
    "src/audio_sdl3.cpp"
    "src/input_sdl3.cpp"
    "src/input_game_controller_sdl3.cpp"
    "src/input_joystick_sdl3.cpp"
    "src/input_keyboard_sdl3.cpp"
    "src/input_mouse_sdl3.cpp"
    "src/sdl3_plugin.cpp"
    "src/sdl3_gl_context.cpp"
    "src/sdl3_rw_ops.cpp"
    "src/sdl3_window.cpp"
    "src/system_sdl3.cpp"

    "../sdl/src/sdl_save.cpp"
    )

set(HEADERS
    "src/audio_sdl3.h"
    "src/input_sdl3.h"
    "src/input_game_controller_sdl3.h"
    "src/input_joystick_sdl3.h"
    "src/input_keyboard_sdl3.h"
    "src/input_mouse_sdl3.h"
    "src/sdl3_gl_context.h"
    "src/sdl3_rw_ops.h"
    "src/sdl3_window.h"
    "src/system_sdl3.h"

    "../sdl/src/sdl_save.h"
    )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-sdl3 ${SOURCES} ${HEADERS})
target_link_libraries(halley-sdl3 halley-engine SDL3::SDL3)
