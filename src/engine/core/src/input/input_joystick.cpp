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

JoystickType InputJoystick::getJoystickType() const
{
	return JoystickType::Generic;
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
	bool isXbox = getJoystickType() == JoystickType::Xbox;
	switch (position) {
		case JoystickButtonPosition::FaceTop: return isXbox ? 3 : 0;
		case JoystickButtonPosition::FaceRight: return isXbox ? 1 : 1;
		case JoystickButtonPosition::FaceBottom: return isXbox ? 0 : 2;
		case JoystickButtonPosition::FaceLeft: return isXbox ? 2 : 3;
		case JoystickButtonPosition::Select: return 4;
		case JoystickButtonPosition::Start: return 5;
		case JoystickButtonPosition::BumperLeft: return 6;
		case JoystickButtonPosition::BumperRight: return 7;
		case JoystickButtonPosition::LeftStick: return 8;
		case JoystickButtonPosition::RightStick: return 9;
		case JoystickButtonPosition::PlatformAcceptButton: return 0;
		case JoystickButtonPosition::PlatformCancelButton: return 1;
		default: throw Exception("Invalid parameter", HalleyExceptions::Input);
	}
}

void InputJoystick::update(Time t)
{
	updateVibration(t);
}

void InputJoystick::vibrate(spInputVibration vibration)
{
	vibs.push_back(vibration);
}

void InputJoystick::stopVibrating()
{
	vibs.clear();
}

bool InputJoystick::isEnabled() const
{
	return enabled;
}

void InputJoystick::setEnabled(bool e)
{
	if (enabled != e) {
		enabled = e;
		if (enabled) {
			lastTime = std::chrono::steady_clock::now();
		}
	}
}

void InputJoystick::clearAxes()
{
	for (auto& axis: axes) {
		axis = 0;
	}
}

void InputJoystick::setVibration(float, float)
{
}

void InputJoystick::updateVibration(Time t)
{
	/*
	auto curTime = std::chrono::steady_clock::now();
	std::chrono::duration<double> elapsed = curTime - lastTime;
	Time t = elapsed.count();
	lastTime = curTime;
	*/

	if (!isEnabled()) {
		stopVibrating();
		return;
	}
	
	float high = 0;
	float low = 0;
	
	Vector<spInputVibration> vibs2 = std::move(vibs);
	vibs.clear();
	
	for (size_t i = 0; i < vibs2.size(); i++) {
		float h = 0;
		float l = 0;
		bool result = vibs2[i]->getState(t, h, l);
		if (result) {
			vibs.push_back(vibs2[i]);
		}
		high += h;
		low += l;
	}
	setVibration(low, high);
}
