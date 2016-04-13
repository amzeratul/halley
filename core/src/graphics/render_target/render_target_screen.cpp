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

#include "render_target_screen.h"

using namespace Halley;

ScreenRenderTarget::ScreenRenderTarget(Rect4f viewPort)
	: viewPort(viewPort)
{
}

std::unique_ptr<RenderTarget> ScreenRenderTarget::makeSubArea(Rect4f area)
{
	Rect4f newViewPort(viewPort.getP1() + area.getP1(), area.getWidth(), area.getHeight());
	return std::make_unique<ScreenRenderTarget>(newViewPort);
}

void ScreenRenderTarget::bind()
{
}

void ScreenRenderTarget::unbind()
{
}
