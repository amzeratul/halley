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

#if defined(WITH_OPENGL)
	#include "gl_core_3_3.h"
	#if defined(__APPLE__)
		#include <OpenGL/gl.h>
	#else
		#include <GL/gl.h>
	#endif
#elif defined(WITH_OPENGL_ES2)
	#define GL_GLEXT_PROTOTYPES
	#include <GLES2/gl2.h>
#elif defined(WITH_OPENGL_ES)
	#include <GLES/gl.h>
#endif

#include "gl_utils.h"
