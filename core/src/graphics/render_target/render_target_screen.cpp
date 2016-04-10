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

void ScreenRenderTarget::bind(int /*attachment*/, bool /*preserveCurrent*/)
{
}

void ScreenRenderTarget::unbind()
{
}

Vector2f ScreenRenderTarget::getSize() const
{
	// TODO
	return Vector2f();
	//return Video::getDisplaySize();
}

Vector2f ScreenRenderTarget::getViewSize() const
{
	// TODO
	return Vector2f();
	//return Video::getVirtualSize();
}

Vector2f ScreenRenderTarget::getOrigin() const
{
	// TODO
	return Vector2f();
	//return Video::getOrigin();
}

