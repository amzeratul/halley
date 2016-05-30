/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#pragma once

namespace Halley {} // Get GitHub to realise this is C++ :3

#define WITH_OPENGL

#include "gl_core_3_3.h"
#if defined(WITH_OPENGL)
	#include <SDL_opengl.h>
#elif defined(WITH_OPENGL_ES2)
	#include <SDL_opengles2.h>
#elif defined(WITH_OPENGL_ES)
	#include <SDL_opengles.h>
#endif

#include "gl_utils.h"
