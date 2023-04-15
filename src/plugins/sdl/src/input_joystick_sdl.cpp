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
#include <SDL_joystick.h>
#include "input_joystick_sdl.h"
#include <iostream>
#include <halley/utils/utils.h>

#include "halley/support/console.h"

using namespace Halley;


InputJoystickSDL::InputJoystickSDL(int number)
	: index(number)
{
	// Open
	auto joy = SDL_JoystickOpen(index);
	if (!joy) {
		throw Exception("Could not open Joystick", HalleyExceptions::InputPlugin);
	}
	joystick = joy;
	id = SDL_JoystickInstanceID(joy);
	name = String(SDL_JoystickName(joy))
		+ " [" + toString(SDL_JoystickGetDeviceVendor(index), 16, 4)
		+ ":" + toString(SDL_JoystickGetDeviceProduct(index), 16, 4)
		+ "]";

	// Axes
	axes.resize(SDL_JoystickNumAxes(joy));
	axisAdjust = &defaultAxisAdjust;

	// Hats
	const int nHats = SDL_JoystickNumHats(joy);
	for (int i = 0; i < nHats; i++) {
		hats.push_back(spInputButtonBase(new InputJoystickHatSDL));
	}

	// Buttons
	baseButtons = SDL_JoystickNumButtons(joy);
	init(std::min(baseButtons + 4, 512));

	std::cout << "\tInitialized SDL Joystick: \"" << ConsoleColour(Console::DARK_GREY) << getName() << ConsoleColour() << "\".\n";
}

InputJoystickSDL::~InputJoystickSDL()
{
	close();
}

void InputJoystickSDL::update(Time t)
{
	clearPresses();

	if (!hats.empty()) {
		for (int i = 0; i < 4; ++i) {
			onButtonStatus(baseButtons + i, hats[0]->isButtonDown(i));
		}
	}
	InputJoystick::update(t);
}

void InputJoystickSDL::close()
{
	if (joystick) {
		std::cout << "\tRemoved SDL Joystick: \"" << ConsoleColour(Console::DARK_GREY) << getName() << ConsoleColour() << "\".\n";
		doSetVibration(0, 0);
		SDL_JoystickClose(static_cast<SDL_Joystick*>(joystick));
		joystick = nullptr;
		id = -1;
	}
}

int InputJoystickSDL::getSDLJoystickId() const
{
	return id;
}

int InputJoystickSDL::getButtonAtPosition(JoystickButtonPosition position) const
{
	switch (position) {
#if defined(linux)
		// Linux 'xpad' driver defaults
		case JoystickButtonPosition::FaceTop: return 3;
		case JoystickButtonPosition::FaceRight: return 1;
		case JoystickButtonPosition::FaceBottom: return 0;
		case JoystickButtonPosition::FaceLeft: return 2;
		case JoystickButtonPosition::Select: return 6;
		case JoystickButtonPosition::Start: return 7;
		case JoystickButtonPosition::BumperLeft: return 4;
		case JoystickButtonPosition::BumperRight: return 5;
		case JoystickButtonPosition::LeftStick: return 9;
		case JoystickButtonPosition::RightStick: return 10;
		case JoystickButtonPosition::PlatformAcceptButton: return 0;
		case JoystickButtonPosition::PlatformCancelButton: return 1;
		case JoystickButtonPosition::TriggerLeft: return 15;
		case JoystickButtonPosition::TriggerRight: return 16;
		case JoystickButtonPosition::System: return 17;
#elif defined(__APPLE__)
		// Mac OS '360Controller' driver defaults
		case JoystickButtonPosition::FaceTop: return 3;
		case JoystickButtonPosition::FaceRight: return 1;
		case JoystickButtonPosition::FaceBottom: return 0;
		case JoystickButtonPosition::FaceLeft: return 2;
		case JoystickButtonPosition::Select: return 9;
		case JoystickButtonPosition::Start: return 8;
		case JoystickButtonPosition::BumperLeft: return 4;
		case JoystickButtonPosition::BumperRight: return 5;
		case JoystickButtonPosition::LeftStick: return 6;
		case JoystickButtonPosition::RightStick: return 7;
		case JoystickButtonPosition::PlatformAcceptButton: return 0;
		case JoystickButtonPosition::PlatformCancelButton: return 1;
		case JoystickButtonPosition::TriggerLeft: return 15;
		case JoystickButtonPosition::TriggerRight: return 16;
		case JoystickButtonPosition::System: return 17;
#else
		// Windows defaults
		case JoystickButtonPosition::FaceTop: return 3;
		case JoystickButtonPosition::FaceRight: return 1;
		case JoystickButtonPosition::FaceBottom: return 0;
		case JoystickButtonPosition::FaceLeft: return 2;
		case JoystickButtonPosition::Select: return 8;
		case JoystickButtonPosition::Start: return 9;
		case JoystickButtonPosition::BumperLeft: return 4;
		case JoystickButtonPosition::BumperRight: return 5;
		case JoystickButtonPosition::LeftStick: return 6;
		case JoystickButtonPosition::RightStick: return 7;
		case JoystickButtonPosition::PlatformAcceptButton: return 0;
		case JoystickButtonPosition::PlatformCancelButton: return 1;
		case JoystickButtonPosition::TriggerLeft: return 10;
		case JoystickButtonPosition::TriggerRight: return 11;
		case JoystickButtonPosition::System: return 17;
#endif
		case JoystickButtonPosition::DPadUp: return baseButtons;
		case JoystickButtonPosition::DPadRight: return baseButtons + 1;
		case JoystickButtonPosition::DPadDown: return baseButtons + 2;
		case JoystickButtonPosition::DPadLeft: return baseButtons + 3;
		default: throw Exception("Invalid parameter", HalleyExceptions::Input);
	}
}

std::string_view InputJoystickSDL::getName() const
{
	return name;
}

int InputJoystickSDL::getSDLAxisIndex(int axis)
{
#if defined(linux) || defined(__APPLE__)
	if (axes.size() < 6) {
		return axis;
	}
	switch (axis) {
		case 0: return SDL_CONTROLLER_AXIS_LEFTX;
		case 1: return SDL_CONTROLLER_AXIS_LEFTY;
		case 2: return SDL_CONTROLLER_AXIS_TRIGGERLEFT;
		case 3: return SDL_CONTROLLER_AXIS_RIGHTX;
		case 4: return SDL_CONTROLLER_AXIS_RIGHTY;
		case 5: return SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
		default: return axis;
	}
#else
	return axis;
#endif
}

void InputJoystickSDL::processAxisEvent(int axis, float value)
{
	axes[axis] = value;

	if (axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT) {
		onButtonStatus(getButtonAtPosition(JoystickButtonPosition::TriggerLeft), value > 0.5f);
	} else if (axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT) {
		onButtonStatus(getButtonAtPosition(JoystickButtonPosition::TriggerRight), value > 0.5f);
	}
}

void InputJoystickSDL::processEvent(const SDL_Event& _event)
{
	switch (_event.type) {
		case SDL_JOYAXISMOTION:
			{
				SDL_JoyAxisEvent event = _event.jaxis;
				processAxisEvent(getSDLAxisIndex(event.axis), clamp(axisAdjust(event.value / 32767.0f), -1.0f, 1.0f));
				break;
			}
		case SDL_JOYBUTTONDOWN:
			{
				SDL_JoyButtonEvent event = _event.jbutton;
				onButtonPressed(event.button);
				break;
			}
		case SDL_JOYBUTTONUP:
			{
				SDL_JoyButtonEvent event = _event.jbutton;
				onButtonReleased(event.button);
				break;
			}
		case SDL_JOYHATMOTION:
			{
				SDL_JoyHatEvent event = _event.jhat;
				std::dynamic_pointer_cast<InputJoystickHatSDL>(hats[event.hat])->processEvent(_event);
				break;
			}
		case SDL_JOYBALLMOTION:
			{
				break;
			}
	}
}

void InputJoystickSDL::doSetVibration(float low, float high)
{
	if (joystick) {
		SDL_JoystickRumble(static_cast<SDL_Joystick*>(joystick), static_cast<uint16_t>(low * 65535), static_cast<uint16_t>(high * 65535), 100);
	}
}


void InputJoystickHatSDL::processEvent(const SDL_Event& _event)
{
	SDL_JoyHatEvent event = _event.jhat;
	int value = event.value;

	for (int i=0; i<4; i++) {
		if (value & (1 << i)) {
			onButtonPressed(i);
		} else {
			onButtonReleased(i);
		}
	}
}
