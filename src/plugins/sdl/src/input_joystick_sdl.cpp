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

using namespace Halley;


InputJoystickSDL::InputJoystickSDL(int number)
	: index(number)
{
	// Open
	auto joy = SDL_JoystickOpen(index);
	if (!joy) throw Exception("Could not open Joystick", HalleyExceptions::InputPlugin);
	joystick = joy;

	// Axes
	axes.resize(SDL_JoystickNumAxes(joy));
	axisAdjust = &defaultAxisAdjust;

	// Hats
	int nHats = SDL_JoystickNumHats(joy);
	for (int i=0; i<nHats; i++) {
		hats.push_back(spInputButtonBase(new InputJoystickHatSDL));
	}

	// Buttons
	int buttons = SDL_JoystickNumButtons(joy);
	init(std::max(buttons, 64));
}

InputJoystickSDL::~InputJoystickSDL()
{
	if (joystick) {
		SDL_JoystickClose(reinterpret_cast<SDL_Joystick*>(joystick));
		joystick = nullptr;
	}
}

void InputJoystickSDL::update(Time t)
{
	clearPresses();
	InputJoystick::update(t);
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
#else
		// Windows defaults
		case JoystickButtonPosition::FaceTop: return 0;
		case JoystickButtonPosition::FaceRight: return 1;
		case JoystickButtonPosition::FaceBottom: return 2;
		case JoystickButtonPosition::FaceLeft: return 3;
		case JoystickButtonPosition::Select: return 4;
		case JoystickButtonPosition::Start: return 5;
		case JoystickButtonPosition::BumperLeft: return 6;
		case JoystickButtonPosition::BumperRight: return 7;
		case JoystickButtonPosition::LeftStick: return 8;
		case JoystickButtonPosition::RightStick: return 9;
		case JoystickButtonPosition::PlatformAcceptButton: return 2;
		case JoystickButtonPosition::PlatformCancelButton: return 1;
		case JoystickButtonPosition::TriggerLeft: return 15;
		case JoystickButtonPosition::TriggerRight: return 16;
#endif
		default: throw Exception("Invalid parameter", HalleyExceptions::Input);
	}
}

std::string InputJoystickSDL::getName() const
{
	return SDL_JoystickName(reinterpret_cast<SDL_Joystick*>(joystick));
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
