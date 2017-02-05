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

#include <cmath>
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
	float absVal = std::abs(value);
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

std::shared_ptr<InputDevice> InputJoystick::getHat(int n)
{
	return hats.at(n);
}

int InputJoystick::getButtonAtPosition(JoystickButtonPosition position) const
{
	bool isXbox = getType() == JoystickType::Xbox;
	switch (position) {
		case JoystickButtonPosition::FaceTop: return isXbox ? 3 : 0;
		case JoystickButtonPosition::FaceRight: return isXbox ? 1 : 1;
		case JoystickButtonPosition::FaceBottom: return isXbox ? 0 : 2;
		case JoystickButtonPosition::FaceLeft: return isXbox ? 2 : 3;
		case JoystickButtonPosition::Select: return 4;
		case JoystickButtonPosition::Start: return 5;
		case JoystickButtonPosition::BumperLeft: return 6;
		case JoystickButtonPosition::BumperRight: return 7;
		default: throw Exception("Invalid parameter");
	}
}
