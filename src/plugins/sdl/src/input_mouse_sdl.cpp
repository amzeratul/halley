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
{
	init(5);
}

void InputMouseSDL::processEvent(const SDL_Event& event, const std::function<Vector2f(Vector2i)>& remap)
{
	switch (event.type) {
	case SDL_MOUSEMOTION:
		{
			Vector2i p;
			p.x = event.motion.x;
			p.y = event.motion.y;
			relMove = Vector2f(Vector2i(event.motion.xrel, event.motion.yrel));
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

void InputMouseSDL::updateRemap(const std::function<Vector2f(Vector2i)>& remap) {
	int x, y;
	SDL_GetMouseState(&x, &y);
	pos = remap(Vector2i(x, y));
}

Vector2f InputMouseSDL::getPosition() const
{
	return pos;
}

void InputMouseSDL::setPosition(Vector2f position)
{
	SDL_WarpMouseInWindow(nullptr, static_cast<int>(position.x), static_cast<int>(position.y));
}

int InputMouseSDL::getWheelMove() const
{
	return wheelMove;
}

void InputMouseSDL::update()
{
	clearPresses();
}

InputType InputMouseSDL::getInputType() const
{
	return InputType::Mouse;
}

std::string_view InputMouseSDL::getName() const
{
	return "Mouse";
}

float InputMouseSDL::getAxis(int n)
{
	if (n == 0) {
		return relMove.x;
		//return pos.x - prevPos.x;
	} else if (n == 1) {
		return relMove.y;
		//return pos.y - prevPos.y;
	} else if (n == 2) {
		return pos.x;
	} else if (n == 3) {
		return pos.y;
	} else {
		return 0.0f;
	}
}

void InputMouseSDL::clearPresses()
{
	InputButtonBase::clearPresses();
	wheelMove = 0;
	if (!isMouseTrapped) {
		prevPos = pos;
	}
	relMove = {};
}

void InputMouseSDL::setDeltaPos(Vector2i deltaPos)
{
	this->prevPos = Vector2f((float)deltaPos.x, (float)deltaPos.y);
}

void InputMouseSDL::setMouseTrapped(bool isTrapped)
{
	isMouseTrapped = isTrapped;
}