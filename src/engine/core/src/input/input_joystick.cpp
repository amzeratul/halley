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
#include "halley/input/input_joystick.h"
using namespace Halley;


float InputJoystick::getAxis(int n) const
{
	if (n >= 0 && n < int(axes.size())) {
		return axisAdjust(axes[n]);
	}
	return 0;
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

std::string_view InputJoystick::getName() const
{
	return "Joystick";
}

JoystickType InputJoystick::getJoystickType() const
{
	return JoystickType::Generic;
}

InputType InputJoystick::getInputType() const
{
	return InputType::Gamepad;
}

size_t InputJoystick::getNumberAxes() const
{
	return axes.size();
}

size_t InputJoystick::getNumberHats() const
{
	return hats.size();
}

std::shared_ptr<InputDevice> InputJoystick::getHat(int n) const
{
	return hats.at(n);
}

void InputJoystick::update(Time t)
{
	updateVibration(t);
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

void InputJoystick::clearPresses()
{
	InputButtonBase::clearPresses();
	for (auto& hat: hats) {
		hat->clearPresses();
	}
}

bool InputJoystick::isAnyButtonPressed() const
{
	for (auto& hat: hats) {
		if (hat->isAnyButtonPressed()) {
			return true;
		}
	}
	return InputButtonBase::isAnyButtonPressed();
}

bool InputJoystick::isAnyButtonReleased() const
{
	for (auto& hat: hats) {
		if (hat->isAnyButtonReleased()) {
			return true;
		}
	}
	return InputButtonBase::isAnyButtonReleased();
}

bool InputJoystick::isAnyButtonDown() const
{
	for (auto& hat: hats) {
		if (hat->isAnyButtonDown()) {
			return true;
		}
	}
	return InputButtonBase::isAnyButtonDown();
}

void InputJoystick::setAxisAdjust(std::function<float(float)> f)
{
	axisAdjust = f;
}

String InputJoystick::getButtonName(int code) const
{
	if (const auto pos = getPositionForButton(code)) {
		return getJoystickButtonName(getJoystickType(), *pos);
	}
	return "";
}

String InputJoystick::getAxisName(int index) const
{
	if (const auto pos = getPositionForAxis(index)) {
		return getJoystickAxisName(getJoystickType(), *pos);
	}
	return "";
}

std::string_view InputJoystick::getJoystickButtonName(JoystickType type, JoystickButtonPosition position)
{
	if (type == JoystickType::Xbox || type == JoystickType::Generic) {
		auto buttons = std::to_array({
			"xbox_a",
			"xbox_b",
			"xbox_x",
			"xbox_y",
			"xbox_lb",
			"xbox_rb",
			"xbox_lt",
			"xbox_rt",
			"xbox_lsb",
			"xbox_rsb",
			"xbox_back",
			"xbox_start",
			"xbox_dpad_up",
			"xbox_dpad_right",
			"xbox_dpad_down",
			"xbox_dpad_left",
			"xbox_guide",
			"xbox_share",
			"",
			"xbox_paddle1",
			"xbox_paddle2",
			"xbox_paddle3",
			"xbox_paddle4",
			"xbox_a",
			"xbox_b",
		});
		return buttons[static_cast<int>(position)];
	} else if (type == JoystickType::Playstation) {
		auto buttons = std::to_array({
			"playstation_cross",
			"playstation_circle",
			"playstation_square",
			"playstation_triangle",
			"playstation_l1",
			"playstation_r1",
			"playstation_l2",
			"playstation_r2",
			"playstation_l3",
			"playstation_r3",
			"playstation_touchpad",
			"playstation_option",
			"playstation_dpad_up",
			"playstation_dpad_right",
			"playstation_dpad_down",
			"playstation_dpad_left",
			"playstation_guide",
			"playstation_share",
			"",
			"",
			"",
			"",
			"",
			"playstation_cross",
			"playstation_circle",
		});
		return buttons[static_cast<int>(position)];
	} else if (type == JoystickType::SwitchFull) {
		auto buttons = std::to_array({
			"switch_b",
			"switch_a",
			"switch_y",
			"switch_x",
			"switch_l",
			"switch_r",
			"switch_zl",
			"switch_zr",
			"switch_lsb",
			"switch_rsb",
			"switch_minus",
			"switch_plus",
			"switch_dpad_up",
			"switch_dpad_right",
			"switch_dpad_down",
			"switch_dpad_left",
			"switch_guide",
			"switch_capture",
			"",
			"",
			"",
			"",
			"",
			"switch_a",
			"switch_b",
		});
		return buttons[static_cast<int>(position)];
	} else if (type == JoystickType::SwitchLeftJoycon || type == JoystickType::SwitchRightJoycon) {
		auto buttons = std::to_array({
			"switch_alt_b",
			"switch_alt_a",
			"switch_alt_y",
			"switch_alt_x",
			"switch_alt_sl",
			"switch_alt_sr",
			"switch_zl",
			"switch_zr",
			"switch_lsb",
			"switch_rsb",
			"switch_minus",
			"switch_plus",
			"switch_dpad_up",
			"switch_dpad_right",
			"switch_dpad_down",
			"switch_dpad_left",
			"switch_guide",
			"switch_capture",
			"",
			"",
			"",
			"",
			"",
			"switch_a",
			"switch_b",
		});
		return buttons[static_cast<int>(position)];
	}
	return "";
}

std::string_view InputJoystick::getJoystickAxisName(JoystickType type, JoystickAxisPosition position)
{
	if (position == JoystickAxisPosition::TriggerLeft) {
		return getJoystickButtonName(type, JoystickButtonPosition::TriggerLeft);
	} else if (position == JoystickAxisPosition::TriggerRight) {
		return getJoystickButtonName(type, JoystickButtonPosition::TriggerRight);
	} else if (position >= JoystickAxisPosition::LeftStickX && position <= JoystickAxisPosition::RightStickY) {
		auto axes = std::to_array({
			"left_stick_x",
			"left_stick_y",
			"right_stick_x",
			"right_stick_y",
		});
		return axes[static_cast<int>(position)];
	}
	return "";
}

String InputJoystick::getJoystickAxisName(JoystickType type, JoystickAxisPosition position, JoystickAxisDirection direction)
{
	return getJoystickAxisName(type, position) + String(direction == JoystickAxisDirection::Positive ? "+" : "-");
}

void InputJoystick::doSetVibration(float low, float high)
{
}

std::pair<float, float> InputJoystick::getVibration() const
{
	return { curLowVib, curHighVib };
}

void InputJoystick::setVibration(float low, float high)
{
	baseHighVib = high;
	baseLowVib = low;
}

void InputJoystick::vibrate(spInputVibration vibration)
{
	vibs.push_back(vibration);
}

void InputJoystick::stopVibrating()
{
	vibs.clear();
	curHighVib = curLowVib = 0.0f;
	baseHighVib = baseLowVib = 0.0f;
}

void InputJoystick::updateVibration(Time t)
{
	float high = 0;
	float low = 0;

	if (isEnabled()) {
		high = baseHighVib;
		low = baseLowVib;
		
		for (size_t i = 0; i < vibs.size();) {
			float h = 0;
			float l = 0;
			const bool result = vibs[i]->getState(t, h, l);

			if (result) {
				++i;
			} else {
				vibs.erase(vibs.begin() + i);
			}
			
			high += h;
			low += l;
		}
	}

	if (curLowVib != low || curHighVib != high) {
		curLowVib = low;
		curHighVib = high;
		doSetVibration(clamp(low, 0.0f, 1.0f), clamp(high, 0.0f, 1.0f));
	}
}
