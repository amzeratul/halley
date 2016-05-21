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

#include <SDL_joystick.h>
#include "input/input_joystick.h"
using namespace Halley;


float InputJoystick::getAxis(int n)
{
	try {
		return axisAdjust(axes.at(n));
	} catch (...) {
		return 0;
	}
}

float InputJoystick::defaultAxisAdjust(float value)
{
	float absVal = abs(value);
	float sign = value >= 0 ? 1.0f : -1.0f;

	const float cutOff = 0.2f;
	if (absVal < cutOff) absVal = 0;
	else absVal = (absVal-cutOff) / (1.0f - cutOff);

	return absVal * sign;
}

size_t InputJoystick::getNumberAxes()
{
	return axes.size();
}

size_t InputJoystick::getNumberHats()
{
	return hats.size();
}

InputDevice& InputJoystick::getHat(int n)
{
	return *hats.at(n);
}

int InputJoystick::getButtonAtPosition(char c) const
{
	bool is360 = getType() == JOYSTICK_360;
	switch (c) {
		case 'N': return is360 ? 3 : 0;
		case 'E': return is360 ? 1 : 1;
		case 'S': return is360 ? 0 : 2;
		case 'W': return is360 ? 2 : 3;
		case 'L': return 4;
		case 'R': return 5;
		default: throw Exception("Invalid parameter");
	}
}
