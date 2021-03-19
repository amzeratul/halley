#pragma once

#include <halley/core/halley_core.h>
#include <halley/core/game/halley_main.h>
#include <halley/halley_entity.h>
#include <halley/halley_utils.h>
#include <halley/net/halley_net.h>
#include <halley/audio/halley_audio.h>
#include <halley/lua/halley_lua.h>
#include <halley/ui/halley_ui.h>
#include <halley/editor_extensions/halley_editor_extensions.h>

#ifdef __glew_h__
#error "Glew cannot be included in halley.hpp"
#endif

#ifdef _SDL_H
#error "SDL cannot be included in halley.hpp"
#endif
