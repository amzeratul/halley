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

#include "halley/maths/rect.h"

namespace Halley {
	class Painter;

	class RenderTarget {
	public:
		virtual ~RenderTarget() {}

		virtual Rect4i getViewPort() const = 0;
		
		virtual bool getProjectionFlipVertical() const { return false; };
		virtual bool getViewportFlipVertical() const { return false; };

		virtual void onBind(Painter&) {}
		virtual void onUnbind(Painter&) {}
		virtual void onStartDrawCall(Painter&) {}
		virtual void onEndDrawCall(Painter&) {}
	};
}
