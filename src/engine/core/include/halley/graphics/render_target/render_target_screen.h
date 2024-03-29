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

#include "render_target.h"

namespace Halley {
	class ScreenRenderTarget : public RenderTarget {
	public:
		ScreenRenderTarget(Rect4i viewPort) : viewPort(viewPort) {}
		virtual ~ScreenRenderTarget() {}

		String getName() const override { return "Screen"; }

		Rect4i getViewPort() const override { return viewPort; }

		bool hasColourBuffer(int attachmentNumber) const override { return true; }
		bool hasDepthBuffer() const override { return true; }
	
	protected:
		Vector2f size;
		Rect4i viewPort;
	};
}
