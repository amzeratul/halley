#pragma once

namespace Halley {} // Get GitHub to realise this is C++ :3

#include "halley/api/halley_api.h"

#include "entry/entry_point.h"

#include "halley/game/core.h"
#include "halley/game/environment.h"
#include "halley/game/frame_data.h"
#include "halley/game/game.h"
#include "halley/game/game_console.h"
#include "halley/game/game_platform.h"

#include "halley/graph/base_graph_enums.h"
#include "halley/graph/base_graph_gizmo.h"
#include "halley/graph/base_graph_node.h"
#include "halley/graph/base_graph_renderer.h"
#include "halley/graph/base_graph.h"

#include "halley/graphics/blend.h"
#include "halley/graphics/painter.h"
#include "halley/graphics/render_context.h"
#include "halley/graphics/shader.h"
#include "halley/graphics/texture.h"
#include "halley/graphics/texture_descriptor.h"

#include "halley/graphics/material/material.h"
#include "halley/graphics/material/material_definition.h"
#include "halley/graphics/material/material_parameter.h"

#include "halley/graphics/mesh/mesh.h"
#include "halley/graphics/mesh/mesh_animation.h"
#include "halley/graphics/mesh/mesh_renderer.h"

#include "halley/graphics/movie/movie_player.h"

#include "halley/graphics/render_target/render_graph.h"
#include "halley/graphics/render_target/render_graph_definition.h"
#include "halley/graphics/render_target/render_graph_node.h"
#include "halley/graphics/render_target/render_surface.h"
#include "halley/graphics/render_target/render_target.h"
#include "halley/graphics/render_target/render_target_screen.h"
#include "halley/graphics/render_target/render_target_texture.h"

#include "halley/graphics/text/font.h"
#include "halley/graphics/text/text_renderer.h"

#include "halley/graphics/sprite/animation.h"
#include "halley/graphics/sprite/animation_player.h"
#include "halley/graphics/sprite/particles.h"
#include "halley/graphics/sprite/sprite.h"
#include "halley/graphics/sprite/sprite_painter.h"
#include "halley/graphics/sprite/sprite_sheet.h"

#include "halley/graphics/window.h"

#include "halley/input/input_exclusive.h"
#include "halley/input/input_joystick.h"
#include "halley/input/input_keyboard.h"
#include "halley/input/input_keys.h"
#include "halley/input/input_manual.h"
#include "halley/input/input_touch.h"
#include "halley/input/input_virtual.h"
#include "halley/input/text_input_capture.h"
#include "halley/input/text_input_data.h"

#include "halley/resources/asset_database.h"
#include "halley/resources/asset_pack.h"
#include "halley/resources/resources.h"
#include "halley/resources/resource_locator.h"
#include "halley/resources/resource_reference.h"

#include "halley/stage/stage.h"

#include "halley/devcon/devcon_client.h"
#include "halley/devcon/devcon_server.h"

#include "version/version.h"

#include <halley/halley_utils.h>

#include "entity/halley_entity.h"
#include "net/halley_net.h"
#include "audio/halley_audio.h"
#include "ui/halley_ui.h"

#include "storage/options.h"
