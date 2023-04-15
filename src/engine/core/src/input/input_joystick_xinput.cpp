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

#include "halley/input/input_joystick_xinput.h"
#include <iostream>
#include <halley/utils/utils.h>

#include "halley/support/console.h"
#include "halley/support/logger.h"
#ifdef XINPUT_AVAILABLE
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Xinput.h>
#include <sstream>
#include <ctime>
using namespace Halley;

#define XINPUT_GAMEPAD_GUIDE 0x400

#pragma comment(lib, "XInput9_1_0.lib")
//#pragma comment(lib, "XInput.lib")

namespace {
	struct XINPUT_STATE_EX
	{
	    uint32_t eventCount;
	    WORD wButtons;
	    BYTE bLeftTrigger;
	    BYTE bRightTrigger;
	    SHORT sThumbLX;
	    SHORT sThumbLY;
	    SHORT sThumbRX;
	    SHORT sThumbRY;
	};

	struct XINPUT_CAPABILITIES_EX
	{
	    XINPUT_CAPABILITIES Capabilities;
	    WORD vendorId;
	    WORD productId;
	    WORD revisionId;
	    DWORD a4; //unknown
	};

	typedef int (__stdcall* _XInputGetStateEx)(int, XINPUT_STATE_EX*);
	typedef DWORD(_stdcall* _XInputGetCapabilitiesEx)(DWORD a1, DWORD dwUserIndex, DWORD dwFlags, XINPUT_CAPABILITIES_EX* pCapabilities);
	_XInputGetStateEx XInputGetStateExPtr;
	_XInputGetCapabilitiesEx XInputGetCapabilitiesExPtr;

	void initXInputDLL()
	{
		static bool opened = false;
		static HMODULE dll = nullptr;

		if (!opened) {
			opened = true;
			wchar_t systemPath[MAX_PATH];
			GetSystemDirectoryW(systemPath, sizeof(systemPath));
			const auto xinput14Path = StringUTF16(systemPath) + L"\\XInput1_4.dll";
			const auto xinput13Path = StringUTF16(systemPath) + L"\\XInput1_3.dll";
			dll = LoadLibraryW(xinput14Path.c_str());
			if (!dll) {
				dll = LoadLibraryW(xinput13Path.c_str());
			}

			if (dll) {
				XInputGetStateExPtr = reinterpret_cast<_XInputGetStateEx>(GetProcAddress(dll, reinterpret_cast<LPCSTR>(100)));
				XInputGetCapabilitiesExPtr = reinterpret_cast<_XInputGetCapabilitiesEx>(GetProcAddress(dll, reinterpret_cast<LPCSTR>(108)));
			}
		}
	}
	
	DWORD XInputGetStateEx(DWORD index, XINPUT_STATE* state)
	{
		initXInputDLL();
		if (XInputGetStateExPtr) {
			XINPUT_STATE_EX stateEx;
			ZeroMemory(&stateEx, sizeof(stateEx));
			const auto result = XInputGetStateExPtr(index, &stateEx);
			if (result != ERROR_SUCCESS) {
				return result;
			}

			state->dwPacketNumber = stateEx.eventCount;
			state->Gamepad.wButtons = stateEx.wButtons;
			state->Gamepad.bLeftTrigger = stateEx.bLeftTrigger;
			state->Gamepad.bRightTrigger = stateEx.bRightTrigger;
			state->Gamepad.sThumbLX = stateEx.sThumbLX;
			state->Gamepad.sThumbLY = stateEx.sThumbLY;
			state->Gamepad.sThumbRX = stateEx.sThumbRX;
			state->Gamepad.sThumbRY = stateEx.sThumbRY;
			return 0;
		}
		return XInputGetState(index, state);
	}

	DWORD XInputGetCapabilitiesEx(DWORD dwUserIndex, DWORD dwFlags, XINPUT_CAPABILITIES_EX* pCapabilities)
	{
		initXInputDLL();
		if (XInputGetCapabilitiesExPtr) {
			return XInputGetCapabilitiesExPtr(1, dwUserIndex, dwFlags, pCapabilities);
		}
		return 0;
	}
}


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

	// Buttons
	init(17);
}

InputJoystickXInput::~InputJoystickXInput()
{
	doSetVibration(0, 0);
}

std::string_view InputJoystickXInput::getName() const
{
	XINPUT_CAPABILITIES_EX capabilities;
	const auto result = XInputGetCapabilitiesEx(index, 0, &capabilities);
	if (SUCCEEDED(result)) {
		name = "XInput Controller #" + toString(index + 1) + " [" + toString(capabilities.vendorId, 16, 4) + ":" + toString(capabilities.productId, 16, 4) + "]";
	} else {
		name = "XInput Controller #" + toString(index + 1);
	}
	return name;
}


InputType InputJoystickXInput::getInputType() const
{
	return InputType::Gamepad;
}

void InputJoystickXInput::update(Time t)
{
	hats[0]->setParent(shared_from_this());
	clearPresses();

	// If disabled, only check once every 30 steps, since XInputGetState() is fairly expensive
	if (!isEnabled() && cooldown > 0) {
		cooldown--;
		return;
	}

	XINPUT_STATE state;
	ZeroMemory(&state, sizeof(XINPUT_STATE));

	DWORD result = XInputGetStateEx(index, &state);
	if (result == ERROR_SUCCESS) {	// WTF, Microsoft
		if (!isEnabled()) {
			setEnabled(true);
			std::cout << "\tXInput Controller " << ConsoleColour(Console::DARK_GREY) << (index + 1) << ConsoleColour() << " connected: \"" << ConsoleColour(Console::DARK_GREY) << getName() << ConsoleColour() << "\".\n";
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
		onButtonStatus(12, (b & XINPUT_GAMEPAD_DPAD_UP) != 0);
		onButtonStatus(13, (b & XINPUT_GAMEPAD_DPAD_RIGHT) != 0);
		onButtonStatus(14, (b & XINPUT_GAMEPAD_DPAD_DOWN) != 0);
		onButtonStatus(15, (b & XINPUT_GAMEPAD_DPAD_LEFT) != 0);
		onButtonStatus(16, (b & XINPUT_GAMEPAD_GUIDE) != 0);

		// Update hat
		hats[0]->onButtonStatus(0, (b & XINPUT_GAMEPAD_DPAD_UP) != 0);
		hats[0]->onButtonStatus(1, (b & XINPUT_GAMEPAD_DPAD_RIGHT) != 0);
		hats[0]->onButtonStatus(2, (b & XINPUT_GAMEPAD_DPAD_DOWN) != 0);
		hats[0]->onButtonStatus(3, (b & XINPUT_GAMEPAD_DPAD_LEFT) != 0);
	} else {
		if (isEnabled()) {
			setEnabled(false);
			std::cout << "\tXInput Controller " << ConsoleColour(Console::DARK_GREY) << (index + 1) << ConsoleColour() << " disconnected.\n";
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
		case JoystickButtonPosition::DPadUp: return 12;
		case JoystickButtonPosition::DPadRight: return 13;
		case JoystickButtonPosition::DPadDown: return 14;
		case JoystickButtonPosition::DPadLeft: return 15;
		case JoystickButtonPosition::System: return 16;
		default: throw Exception("Invalid parameter", HalleyExceptions::InputPlugin);
	}
}

String InputJoystickXInput::getButtonName(int code) const
{
	auto buttons = std::array<const char*, 17>{
		"xbox_a",
		"xbox_b",
		"xbox_x",
		"xbox_y",
		"xbox_lb",
		"xbox_rb",
		"xbox_lsb",
		"xbox_rsb",
		"xbox_back",
		"xbox_start",
		"xbox_lt",
		"xbox_rt",
		"xbox_dpad_up",
		"xbox_dpad_right",
		"xbox_dpad_down",
		"xbox_dpad_left",
		"xbox_guide"
	};
	return buttons[code];
}


void InputJoystickXInput::doSetVibration(float low, float high)
{
	XINPUT_VIBRATION vibration;
	ZeroMemory(&vibration, sizeof(XINPUT_VIBRATION));
	vibration.wLeftMotorSpeed = static_cast<WORD>(clamp(low * 65535, 0.0f, 65535.0f));
	vibration.wRightMotorSpeed = static_cast<WORD>(clamp(high * 65535, 0.0f, 65535.0f));
	XInputSetState(index, &vibration);
}

#endif
