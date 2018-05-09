#pragma once

namespace Halley {} // Get GitHub to realise this is C++ :3

#include "api/halley_api.h"

#include "game/core.h"
#include "game/environment.h"
#include "game/game.h"

#include "graphics/blend.h"
#include "graphics/painter.h"
#include "graphics/render_context.h"
#include "graphics/shader.h"
#include "graphics/texture.h"
#include "graphics/texture_descriptor.h"

#include "graphics/material/material.h"
#include "graphics/material/material_definition.h"
#include "graphics/material/material_parameter.h"

#include "graphics/movie/movie_player.h"

#include "graphics/render_target/render_target.h"
#include "graphics/render_target/render_target_screen.h"
#include "graphics/render_target/render_target_texture.h"

#include "graphics/text/font.h"
#include "graphics/text/text_renderer.h"

#include "graphics/sprite/animation.h"
#include "graphics/sprite/animation_player.h"
#include "graphics/sprite/sprite.h"
#include "graphics/sprite/sprite_painter.h"
#include "graphics/sprite/sprite_sheet.h"

#include "graphics/window.h"

#include "input/input_joystick.h"
#include "input/input_keys.h"
#include "input/input_manual.h"
#include "input/input_touch.h"
#include "input/input_virtual.h"

#include "resources/asset_database.h"
#include "resources/asset_pack.h"
#include "resources/resources.h"
#include "resources/resource_locator.h"

#include "stage/stage.h"
#include "stage/entity_stage.h"

#include "utils/world_stats.h"


#include <halley/halley_utils.h>
