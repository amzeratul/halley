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

#include "input/input_touch.h"

using namespace Halley;


Halley::InputTouch::InputTouch(Vector2f pos)
	: initialPos(pos)
	, curPos(pos)
	, elapsed(0)
	, pressed(true)
	, released(false)
{
}

void Halley::InputTouch::setPos(Vector2f pos)
{
	curPos = pos;
}

void Halley::InputTouch::update(Time t)
{
	elapsed += t;
	pressed = false;
}

void Halley::InputTouch::setReleased()
{
	released = true;
}

bool Halley::InputTouch::isPressed() const
{
	return pressed;
}

bool Halley::InputTouch::isReleased() const
{
	return released;
}

Halley::Vector2f Halley::InputTouch::getInitialPos() const
{
	return initialPos;
}

Halley::Vector2f Halley::InputTouch::getCurrentPos() const
{
	return curPos;
}

Halley::Time Halley::InputTouch::getTimeElapsed() const
{
	return elapsed;
}
