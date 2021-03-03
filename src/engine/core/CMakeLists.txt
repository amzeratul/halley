project (halley-core)

include_directories(${Boost_INCLUDE_DIR} "include" "include/halley/core" "../utils/include" "../audio/include" "../net/include" "../entity/include")

set(SOURCES
        "src/api/halley_api.cpp"

        "src/dummy/dummy_audio.cpp"
        "src/dummy/dummy_input.cpp"
        "src/dummy/dummy_network.cpp"
        "src/dummy/dummy_movie.cpp"
        "src/dummy/dummy_platform.cpp"
        "src/dummy/dummy_plugins.cpp"
        "src/dummy/dummy_system.cpp"
        "src/dummy/dummy_video.cpp"

        "src/game/core.cpp"
        "src/game/environment.cpp"
        "src/game/game.cpp"
        "src/game/game_console.cpp"
        "src/game/halley_main.cpp"
        "src/game/halley_statics.cpp"
        "src/game/main_loop.cpp"

        "src/graphics/camera.cpp"
        "src/graphics/material/material.cpp"
        "src/graphics/material/material_definition.cpp"
        "src/graphics/material/material_parameter.cpp"
        "src/graphics/mesh/mesh.cpp"
        "src/graphics/mesh/mesh_animation.cpp"
        "src/graphics/mesh/mesh_renderer.cpp"
        "src/graphics/movie/movie_player.cpp"
        "src/graphics/painter.cpp"
        "src/graphics/render_context.cpp"
        "src/graphics/render_target/render_graph.cpp"
        "src/graphics/render_target/render_graph_definition.cpp"
        "src/graphics/render_target/render_graph_node.cpp"
        "src/graphics/render_target/render_surface.cpp"
        "src/graphics/render_target/render_target_texture.cpp"
        "src/graphics/shader.cpp"
        "src/graphics/sprite/animation.cpp"
        "src/graphics/sprite/animation_player.cpp"
        "src/graphics/sprite/particles.cpp"
        "src/graphics/sprite/sprite.cpp"
        "src/graphics/sprite/sprite_painter.cpp"
        "src/graphics/sprite/sprite_sheet.cpp"
        "src/graphics/text/font.cpp"
        "src/graphics/text/text_renderer.cpp"
        "src/graphics/texture.cpp"
        "src/graphics/texture_descriptor.cpp"

        "src/input/input_button_base.cpp"
        "src/input/input_device.cpp"
        "src/input/input_joystick.cpp"
        "src/input/input_joystick_xinput.cpp"
        "src/input/input_keyboard.cpp"
        "src/input/input_manual.cpp"
        "src/input/input_touch.cpp"
        "src/input/input_vibration.cpp"
        "src/input/input_virtual.cpp"
        "src/input/text_input_capture.cpp"
        "src/input/text_input_data.cpp"

        "src/resources/asset_database.cpp"
        "src/resources/asset_pack.cpp"
        "src/resources/resource_collection.cpp"
        "src/resources/resource_filesystem.cpp"
        "src/resources/resource_locator.cpp"
        "src/resources/resource_pack.cpp"
        "src/resources/resource_reference.cpp"
        "src/resources/resources.cpp"
        "src/resources/standard_resources.cpp"

        "src/editor_extensions/component_editor_context.cpp"
        "src/editor_extensions/component_field_parameters.cpp"
        "src/editor_extensions/scene_editor_interface.cpp"

        "src/stage/stage.cpp"

        "src/devcon/devcon_client.cpp"
        "src/devcon/devcon_messages.cpp"
        "src/devcon/devcon_server.cpp"
        )

set(HEADERS

        "src/dummy/dummy_audio.h"
        "src/dummy/dummy_input.h"
        "src/dummy/dummy_plugins.h"
        "src/dummy/dummy_movie.h"
        "src/dummy/dummy_network.h"
        "src/dummy/dummy_platform.h"
        "src/dummy/dummy_system.h"
        "src/dummy/dummy_video.h"

        "include/halley/core/api/audio_api.h"
        "include/halley/core/api/clipboard.h"
        "include/halley/core/api/core_api.h"
        "include/halley/core/api/halley_api.h"
        "include/halley/core/api/halley_api_internal.h"
        "include/halley/core/api/input_api.h"
        "include/halley/core/api/movie_api.h"
        "include/halley/core/api/network_api.h"
        "include/halley/core/api/platform_api.h"
        "include/halley/core/api/save_data.h"
        "include/halley/core/api/system_api.h"
        "include/halley/core/api/video_api.h"

        "include/halley/core/entry/entry_point.h"
        "include/halley/core/entry/game_loader.h"

        "include/halley/core/game/core.h"
        "include/halley/core/game/environment.h"
        "include/halley/core/game/game.h"
        "include/halley/core/game/game_console.h"
        "include/halley/core/game/halley_main.h"
        "include/halley/core/game/halley_statics.h"
        "include/halley/core/game/game_platform.h"
        "include/halley/core/game/main_loop.h"
        
        "include/halley/core/graphics/blend.h"
        "include/halley/core/graphics/camera.h"
        "include/halley/core/graphics/material/material_definition.h"
		"include/halley/core/graphics/material/material_definition.natvis"
        "include/halley/core/graphics/material/material.h"
		"include/halley/core/graphics/material/material.natvis"
        "include/halley/core/graphics/material/material_parameter.h"
        "include/halley/core/graphics/material/uniform_type.h"
        "include/halley/core/graphics/mesh/mesh.h"
        "include/halley/core/graphics/mesh/mesh_animation.h"
        "include/halley/core/graphics/mesh/mesh_renderer.h"
        "include/halley/core/graphics/movie/movie_player.h"
        "include/halley/core/graphics/painter.h"
        "include/halley/core/graphics/render_context.h"
        "include/halley/core/graphics/render_target/render_graph.h"
        "include/halley/core/graphics/render_target/render_graph_definition.h"
        "include/halley/core/graphics/render_target/render_graph_node.h"
        "include/halley/core/graphics/render_target/render_graph_pin_type.h"
        "include/halley/core/graphics/render_target/render_surface.h"
        "include/halley/core/graphics/render_target/render_target.h"
        "include/halley/core/graphics/render_target/render_target_screen.h"
        "include/halley/core/graphics/render_target/render_target_texture.h"
        "include/halley/core/graphics/shader.h"
        "include/halley/core/graphics/sprite/animation.h"
        "include/halley/core/graphics/sprite/animation_player.h"
        "include/halley/core/graphics/sprite/particles.h"
        "include/halley/core/graphics/sprite/sprite.h"
        "include/halley/core/graphics/sprite/sprite_painter.h"
        "include/halley/core/graphics/sprite/sprite_sheet.h"
        "include/halley/core/graphics/text/font.h"
        "include/halley/core/graphics/text/text_renderer.h"
        "include/halley/core/graphics/texture_descriptor.h"
        "include/halley/core/graphics/texture.h"
		"include/halley/core/graphics/window.h"
        
        "include/halley/core/halley_core.h"
        
        "include/halley/core/input/input_button_base.h"
        "include/halley/core/input/input_device.h"
        "include/halley/core/input/input_joystick.h"
        "include/halley/core/input/input_joystick_xinput.h"
        "include/halley/core/input/input_keyboard.h"
        "include/halley/core/input/input_keys.h"
        "include/halley/core/input/input_manual.h"
        "include/halley/core/input/input_touch.h"
        "include/halley/core/input/input_vibration.h"
        "include/halley/core/input/input_virtual.h"
        "include/halley/core/input/text_input_capture.h"
        "include/halley/core/input/text_input_data.h"
        
        "include/halley/core/resources/asset_database.h"
        "include/halley/core/resources/asset_pack.h"
        "include/halley/core/resources/resource_collection.h"
        "include/halley/core/resources/resource_locator.h"
        "include/halley/core/resources/resource_reference.h"
        "include/halley/core/resources/resources.h"
        "include/halley/core/resources/standard_resources.h"

        "src/resources/resource_filesystem.h"
        "src/resources/resource_pack.h"

        "include/halley/core/editor_extensions/component_editor_context.h"
        "include/halley/core/editor_extensions/component_field_parameters.h"
        "include/halley/core/editor_extensions/editor_custom_tools_interface.h"
        "include/halley/core/editor_extensions/scene_editor_interface.h"

        "include/halley/core/stage/stage.h"
        "include/halley/core/stage/stage_id.h"

        "include/halley/core/devcon/devcon_client.h"
        "include/halley/core/devcon/devcon_messages.h"
        "include/halley/core/devcon/devcon_server.h"
        
        "src/prec.h"
        )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (halley-core ${SOURCES} ${HEADERS})
target_link_libraries(halley-core halley-utils halley-audio halley-net halley-entity)

# Hack, there should be no platform specific stuff in core...
if (USE_ANDROID)
    target_link_libraries(halley-core log android)
endif ()
