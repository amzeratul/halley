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

#include <SDL.h>
#include "input_mouse_sdl.h"

using namespace Halley;

InputMouseSDL::InputMouseSDL()
	: wheelMove(0)
{
	init(5);
}

void InputMouseSDL::processEvent(const SDL_Event& event, std::function<Vector2f(Vector2i)> remap)
{
	switch (event.type) {
	case SDL_MOUSEMOTION:
		{
			Vector2i p;
			p.x = event.motion.x;
			p.y = event.motion.y;
			pos = remap(Vector2i(p));
			break;
		}

	case SDL_MOUSEBUTTONDOWN:
		{
			Uint8 button = event.button.button;
			onButtonPressed(button - 1);
			break;
		}

	case SDL_MOUSEBUTTONUP:
		{
			Uint8 button = event.button.button;
			onButtonReleased(button - 1);
			break;
		}

	case SDL_MOUSEWHEEL:
		{
			wheelMove += event.wheel.y;
		}
	}
}

void InputMouseSDL::updateRemap(std::function<Vector2f(Vector2i)> remap) {
	int x, y;
	SDL_GetMouseState(&x, &y);
	pos = remap(Vector2i(x, y));
}

Vector2f InputMouseSDL::getPosition() const
{
	return pos;
}

int InputMouseSDL::getWheelMove() const
{
	return wheelMove;
}

void InputMouseSDL::update()
{
	clearPresses();
	wheelMove = 0;
}
