project (halley-core)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake_modules/")
find_Package(SDL2 REQUIRED)

include_directories(${SDL2_INCLUDE_DIR} "include" "include/halley/core" "../utils/include" "../entity/include")

set(SOURCES
        "src/api/halley_api.cpp"
        "src/core/core_runner.cpp"
        "src/core/environment.cpp"
        "src/graphics/camera.cpp"
        "src/graphics/material/material.cpp"
        "src/graphics/material/material_definition.cpp"
        "src/graphics/material/material_parameter.cpp"
        "src/graphics/painter.cpp"
        "src/graphics/render_context.cpp"
        "src/graphics/render_target/render_target_screen.cpp"
        "src/graphics/render_target/render_target_texture.cpp"
        "src/graphics/shader.cpp"
        "src/graphics/sprite/animation.cpp"
        "src/graphics/sprite/animation_player.cpp"
        "src/graphics/sprite/sprite.cpp"
        "src/graphics/sprite/sprite_painter.cpp"
        "src/graphics/sprite/sprite_sheet.cpp"
        "src/graphics/text/font.cpp"
        "src/graphics/text/text_renderer.cpp"
        "src/graphics/texture.cpp"
        "src/graphics/texture_descriptor.cpp"
        "src/input/input_button_base.cpp"
        "src/input/input.cpp"
        "src/input/input_joystick.cpp"
        "src/input/input_joystick_sdl.cpp"
        "src/input/input_joystick_xinput.cpp"
        "src/input/input_keyboard_concrete.cpp"
        "src/input/input_keys.cpp"
        "src/input/input_manual.cpp"
        "src/input/input_mouse_concrete.cpp"
        "src/input/input_touch.cpp"
        "src/input/input_vibration.cpp"
        "src/input/input_virtual.cpp"
        "src/resources/resource_collection.cpp"
        "src/resources/resource_data_reader.cpp"
        "src/resources/resource_filesystem.cpp"
        "src/resources/resource_locator.cpp"
        "src/resources/resource_pack.cpp"
        "src/resources/resources.cpp"
        "src/resources/standard_resources.cpp"
        "src/sdl/system_sdl.cpp"
        "src/stage/entity_stage.cpp"
        "src/stage/stage.cpp"
        "src/utils/world_stats.cpp"
        )


add_library (halley-core ${SOURCES})