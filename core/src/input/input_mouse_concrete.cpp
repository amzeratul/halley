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
#include "input_mouse_concrete.h"

using namespace Halley;

InputMouseConcrete::InputMouseConcrete()
	: wheelMove(0)
{
	init(5);
}

void InputMouseConcrete::processEvent(const SDL_Event& event)
{
	switch (event.type) {
	case SDL_MOUSEMOTION:
		pos.x = event.motion.x;
		pos.y = event.motion.y;
		break;

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

Halley::Vector2i Halley::InputMouseConcrete::getPosition() const
{
	return pos;
}

int Halley::InputMouseConcrete::getWheelMove() const
{
	return wheelMove;
}

void Halley::InputMouseConcrete::update()
{
	wheelMove = 0;
}
