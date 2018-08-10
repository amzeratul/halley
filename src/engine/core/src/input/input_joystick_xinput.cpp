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

#include "input/input_joystick_xinput.h"
#include <iostream>
#include <halley/utils/utils.h>
#ifdef XINPUT_AVAILABLE
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Xinput.h>
#include <sstream>
#include <ctime>
using namespace Halley;

#pragma comment(lib, "XInput9_1_0.lib")


InputJoystickXInput::InputJoystickXInput(int number)
	: index(number)
	, cooldown(0)
{
	// Axes
	axes.resize(6);
	axisAdjust = &defaultAxisAdjust;

	// Hat
	hats.resize(1);
	hats[0] = std::make_shared<InputButtonBase>(4);
	hats[0]->setParent(this);

	// Buttons
	init(12);
}

InputJoystickXInput::~InputJoystickXInput()
{
	setVibration(0, 0);
}

std::string InputJoystickXInput::getName() const
{
	std::stringstream str;
	str << "XInput Controller #" << (index+1);
	return str.str();
}


void InputJoystickXInput::update(Time t)
{
	clearPresses();

	// If disabled, only check once every 30 steps, since XInputGetState() is fairly expensive
	if (!isEnabled() && cooldown > 0) {
		cooldown--;
		return;
	}

	XINPUT_STATE state;
	ZeroMemory(&state, sizeof(XINPUT_STATE));

	DWORD result = XInputGetState(index, &state);
	if (result == ERROR_SUCCESS) {	// WTF, Microsoft
		if (!isEnabled()) {
			setEnabled(true);
			if (t == 0) std::cout << "\t"; // Just so it aligns on the console during initialization
			std::cout << "XInput controller connected on port " << (index+1) << std::endl;
		}

		auto& gamepad = state.Gamepad;
		
		// Update axes
		axes[0] = gamepad.sThumbLX / 32768.0f;
		axes[1] = -gamepad.sThumbLY / 32768.0f;
		axes[2] = gamepad.sThumbRX / 32768.0f;
		axes[3] = -gamepad.sThumbRY / 32768.0f;
		axes[4] = gamepad.bLeftTrigger / 255.0f;
		axes[5] = gamepad.bRightTrigger / 255.0f;

		// Update buttons
		int b = gamepad.wButtons;
		onButtonStatus(0, (b & XINPUT_GAMEPAD_A) != 0);
		onButtonStatus(1, (b & XINPUT_GAMEPAD_B) != 0);
		onButtonStatus(2, (b & XINPUT_GAMEPAD_X) != 0);
		onButtonStatus(3, (b & XINPUT_GAMEPAD_Y) != 0);
		onButtonStatus(4, (b & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0);
		onButtonStatus(5, (b & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0);
		onButtonStatus(6, (b & XINPUT_GAMEPAD_LEFT_THUMB) != 0);
		onButtonStatus(7, (b & XINPUT_GAMEPAD_RIGHT_THUMB) != 0);
		onButtonStatus(8, (b & XINPUT_GAMEPAD_BACK) != 0);
		onButtonStatus(9, (b & XINPUT_GAMEPAD_START) != 0);
		onButtonStatus(10, axes[4] > 0.5f);
		onButtonStatus(11, axes[5] > 0.5f);

		// Update hat
		hats[0]->onButtonStatus(0, (b & XINPUT_GAMEPAD_DPAD_UP) != 0);
		hats[0]->onButtonStatus(1, (b & XINPUT_GAMEPAD_DPAD_RIGHT) != 0);
		hats[0]->onButtonStatus(2, (b & XINPUT_GAMEPAD_DPAD_DOWN) != 0);
		hats[0]->onButtonStatus(3, (b & XINPUT_GAMEPAD_DPAD_LEFT) != 0);
	} else {
		if (isEnabled()) {
			setEnabled(false);
			std::cout << "XInput controller disconnected from port " << (index+1) << std::endl;
		}
		cooldown = 30;

		// Reset everything
		clearAxes();
		clearPresses();
		hats[0]->clearPresses();
	}

	InputJoystick::update(t);
}

int InputJoystickXInput::getButtonAtPosition(JoystickButtonPosition position) const
{
	switch (position) {
		case JoystickButtonPosition::FaceTop: return 3;
		case JoystickButtonPosition::FaceRight: return 1;
		case JoystickButtonPosition::FaceBottom: return 0;
		case JoystickButtonPosition::FaceLeft: return 2;
		case JoystickButtonPosition::Select: return 8;
		case JoystickButtonPosition::Start: return 9;
		case JoystickButtonPosition::BumperLeft: return 4;
		case JoystickButtonPosition::BumperRight: return 5;
		case JoystickButtonPosition::TriggerLeft: return 10;
		case JoystickButtonPosition::TriggerRight: return 11;
		case JoystickButtonPosition::LeftStick: return 6;
		case JoystickButtonPosition::RightStick: return 7;
		case JoystickButtonPosition::PlatformAcceptButton: return 0;
		case JoystickButtonPosition::PlatformCancelButton: return 1;
		default: throw Exception("Invalid parameter");
	}
}


void InputJoystickXInput::setVibration(float low, float high)
{
	XINPUT_VIBRATION vibration;
	ZeroMemory(&vibration, sizeof(XINPUT_VIBRATION));
	vibration.wLeftMotorSpeed = static_cast<WORD>(clamp(low * 65535, 0.0f, 65535.0f));
	vibration.wRightMotorSpeed = static_cast<WORD>(clamp(high * 65535, 0.0f, 65535.0f));
	XInputSetState(index, &vibration);
}

#endif
