#pragma once

namespace Halley {} // Get GitHub to realise this is C++ :3

#include "halley/api/halley_api.h"

#include "core/core_runner.h"
#include "core/game.h"

#include "halley/graphics/blend.h"
#include "halley/graphics/painter.h"
#include "halley/graphics/render_context.h"
#include "halley/graphics/shader.h"
#include "halley/graphics/texture.h"
#include "halley/graphics/texture_descriptor.h"

#include "halley/graphics/material/material.h"
#include "halley/graphics/material/material_definition.h"
#include "halley/graphics/material/material_parameter.h"

#include "halley/graphics/render_target/render_target.h"
#include "halley/graphics/render_target/render_target_screen.h"
#include "halley/graphics/render_target/render_target_texture.h"

#include "graphics/text/font.h"
#include "graphics/text/text_renderer.h"

#include "graphics/sprite/animation.h"
#include "graphics/sprite/animation_player.h"
#include "graphics/sprite/sprite.h"
#include "graphics/sprite/sprite_painter.h"
#include "graphics/sprite/sprite_sheet.h"

#include "input/input.h"
#include "input/input_joystick.h"
#include "input/input_keys.h"
#include "input/input_keyboard.h"
#include "input/input_manual.h"
#include "input/input_mouse.h"
#include "input/input_touch.h"
#include "input/input_virtual.h"

#include "resources/resources.h"
#include "resources/resource_locator.h"

#include "stage/stage.h"
#include "stage/entity_stage.h"

#include "utils/world_stats.h"
