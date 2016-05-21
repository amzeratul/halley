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

#include "maths/rect.h"
#include "maths/colour.h"
#include "halley_gl.h"
#include "graphics/blend.h"

namespace Halley {

	class Texture;
	class GLInternals;

	class GLUtils {
	public:
		GLUtils();
		GLUtils(GLUtils& other);

		GLUtils& operator=(const GLUtils&) = delete;

		void setBlendType(BlendType type);

		void bindTexture(int id);
		void setTextureUnit(int n);
		void setNumberOfTextureUnits(int n);
		void resetState();

		void setViewPort(Rect4i rect, bool scissor = false);
		Rect4i getViewPort();

		void clear(Colour col);
		static void doGlCheckError(const char* file = "", long line = 0);
		
	private:
		GLInternals& state;
	};
}

#define glCheckError() Halley::GLUtils::doGlCheckError(__FILE__, __LINE__)
