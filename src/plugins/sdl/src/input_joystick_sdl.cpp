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
	if (!joy) throw Exception("Could not open Joystick");
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
	init(buttons);
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

std::string InputJoystickSDL::getName() const
{
	return SDL_JoystickName(reinterpret_cast<SDL_Joystick*>(joystick));
}

void InputJoystickSDL::processEvent(const SDL_Event& _event)
{
	switch (_event.type) {
		case SDL_JOYAXISMOTION:
			{
				SDL_JoyAxisEvent event = _event.jaxis;
				axes[event.axis] = clamp(axisAdjust(event.value / 32767.0f), -1.0f, 1.0f);
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
