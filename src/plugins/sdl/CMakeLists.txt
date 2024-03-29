project (halley-sdl)

include_directories(${Boost_INCLUDE_DIR} ${SDL2_INCLUDE_DIR} "../../engine/utils/include" "../../engine/core/include")

set(SOURCES
    "src/audio_sdl.cpp"
    "src/input_sdl.cpp"
    "src/input_game_controller_sdl.cpp"
    "src/input_joystick_sdl.cpp"
    "src/input_keyboard_sdl.cpp"
    "src/input_mouse_sdl.cpp"
    "src/sdl_plugin.cpp"
    "src/sdl_gl_context.cpp"
    "src/sdl_rw_ops.cpp"
    "src/sdl_save.cpp"
    "src/sdl_window.cpp"
    "src/system_sdl.cpp"
    )

set(HEADERS
    "src/audio_sdl.h"
    "src/input_sdl.h"
    "src/input_game_controller_sdl.h"
    "src/input_joystick_sdl.h"
    "src/input_keyboard_sdl.h"
    "src/input_mouse_sdl.h"
    "src/sdl_gl_context.h"
    "src/sdl_rw_ops.h"
    "src/sdl_save.h"
    "src/sdl_window.h"
    "src/system_sdl.h"
    )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-sdl ${SOURCES} ${HEADERS})
target_link_libraries(halley-sdl halley-engine ${SDL2_LIBRARIES})
