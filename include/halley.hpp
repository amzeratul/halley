#pragma once

#include <halley/core/halley_core.h>
#include <halley/halley_entity.h>
#include <halley/halley_utils.h>
#include <halley/runner/halley_main.h>

#ifdef __glew_h__
#error "Glew cannot be included in halley.hpp"
#endif

#ifdef _SDL_H
#error "SDL cannot be included in halley.hpp"
#endif
