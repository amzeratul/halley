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
#include <halley/maths/utils.h>
#ifdef XINPUT_AVAILABLE
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Xinput.h>
#include <sstream>
#include <ctime>
using namespace Halley;

#pragma comment(lib, "xinput.lib")


InputJoystickXInput::InputJoystickXInput(int number)
	: index(number)
	, enabled(false)
	, cooldown(0)
{
	// Axes
	axes.resize(6);
	axisAdjust = &defaultAxisAdjust;

	// Hat
	hats.resize(1);
	hats[0] = spInputButtonBase(new InputButtonBase(4));

	// Buttons
	init(10);
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
	// If disabled, only check once every 30 steps, since XInputGetState() is fairly expensive
	if (!enabled && cooldown > 0) {
		cooldown--;
		return;
	}

	XINPUT_STATE state;
	ZeroMemory(&state, sizeof(XINPUT_STATE));

	DWORD result = XInputGetState(index, &state);
	if (result == ERROR_SUCCESS) {	// WTF, Microsoft
		if (!enabled) {
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
		axes[4] = gamepad.bLeftTrigger * 2.0f / 255.0f - 1.0f;
		axes[5] = gamepad.bRightTrigger * 2.0f / 255.0f - 1.0f;

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

		// Update hat
		hats[0]->onButtonStatus(0, (b & XINPUT_GAMEPAD_DPAD_UP) != 0);
		hats[0]->onButtonStatus(1, (b & XINPUT_GAMEPAD_DPAD_RIGHT) != 0);
		hats[0]->onButtonStatus(2, (b & XINPUT_GAMEPAD_DPAD_DOWN) != 0);
		hats[0]->onButtonStatus(3, (b & XINPUT_GAMEPAD_DPAD_LEFT) != 0);

		updateVibration(t);
	} else {
		if (enabled) {
			setEnabled(false);
			std::cout << "XInput controller disconnected from port " << (index+1) << std::endl;
		}
		cooldown = 30;

		// Reset everything
		for (int i=0; i<6; i++) axes[i] = 0;
		for (int i=0; i<10; i++) onButtonStatus(i, false);
		for (int i=0; i<4; i++) hats[0]->onButtonStatus(i, false);
	}
}


void InputJoystickXInput::setEnabled(bool e)
{
	if (enabled != e) {
		enabled = e;
		if (enabled) lastTime = clock();
	}
}


void InputJoystickXInput::setVibration(float high, float low)
{
	XINPUT_VIBRATION vibration;
	ZeroMemory(&vibration, sizeof(XINPUT_VIBRATION));
	vibration.wLeftMotorSpeed = static_cast<WORD>(clamp(low * 65535, 0.0f, 65535.0f));
	vibration.wRightMotorSpeed = static_cast<WORD>(clamp(high * 65535, 0.0f, 65535.0f));
	XInputSetState(index, &vibration);
}


void InputJoystickXInput::vibrate(spInputVibration vib)
{
	vibs.push_back(vib);
}

void InputJoystickXInput::stopVibrating()
{
	vibs.clear();
}

void Halley::InputJoystickXInput::updateVibration(Time /*_t*/)
{
	time_t curTime = clock();
	Time t = Time(float(curTime - lastTime) / CLK_TCK);
	lastTime = curTime;
	
	float high = 0;
	float low = 0;
	std::vector<spInputVibration> vibs2 = vibs;
	vibs.clear();
	for (size_t i=0; i<vibs2.size(); i++) {
		float h = 0;
		float l = 0;
		bool result = vibs2[i]->getState(t, h, l);
		if (result) vibs.push_back(vibs2[i]);
		high += h;
		low += l;
	}
	setVibration(high, low);
}

#endif
