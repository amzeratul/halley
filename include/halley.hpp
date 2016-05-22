#pragma once

#include "halley/halley_core.h"
#include "halley/halley_main.h"
#include "halley/halley_entity.h"
#include "halley/halley_utils.h"

#ifdef _MSC_VER
	#ifndef _DEBUG
		#pragma comment(lib, "halley_utils.lib")
		#pragma comment(lib, "halley_entity.lib")
		#pragma comment(lib, "halley_core.lib")
		#pragma comment(lib, "halley_opengl.lib")
	#else
		#pragma comment(lib, "halley_utils_d.lib")
		#pragma comment(lib, "halley_entity_d.lib")
		#pragma comment(lib, "halley_core_d.lib")
		#pragma comment(lib, "halley_opengl_d.lib")
	#endif
#endif

#ifdef __glew_h__
#error "Glew cannot be included in halley.hpp"
#endif

#ifdef _SDL_H
#error "SDL cannot be included in halley.hpp"
#endif
