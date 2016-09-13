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

#include "input_sdl.h"
#include "input_joystick_sdl.h"
#include "input_mouse_sdl.h"
#include "input_keyboard_sdl.h"
#include "halley/core/input/input_touch.h"
#include <SDL.h>
#include "halley/support/console.h"

#ifdef _MSC_VER
#include "halley/core/input/input_joystick_xinput.h"
#endif

using namespace Halley;

InputSDL::InputSDL() = default;
InputSDL::~InputSDL() = default;

void InputSDL::init()
{
	keyboards.push_back(std::unique_ptr<InputKeyboardSDL>(new InputKeyboardSDL()));
	mice.push_back(std::unique_ptr<InputMouseSDL>(new InputMouseSDL()));

	// XInput controllers
#ifdef XINPUT_AVAILABLE
	const bool hasXInput = true;
	for (int i = 0; i<4; i++) {
		auto joy = std::unique_ptr<InputJoystick>(new InputJoystickXInput(i));
		joy->update(Time(0));
		joysticks.push_back(std::move(joy));
	}
#else
	const bool hasXInput = false;
#endif

	// SDL joysticks
	int nJoy = SDL_NumJoysticks();
	for (int i = 0; i<nJoy; i++) {
		auto joy = std::unique_ptr<InputJoystick>(new InputJoystickSDL(i));
		String name = joy->getName();

		bool isXinputController = name.asciiLower().find("xbox 360") != String::npos || name.asciiLower().find("xinput") != String::npos;
		if (!hasXInput || !isXinputController) {
			joysticks.push_back(std::move(joy));
			sdlJoys[i] = dynamic_cast<InputJoystickSDL*>(joysticks.back().get());

			std::cout << "\tInitialized SDL joystick: \"" << ConsoleColour(Console::DARK_GREY) << name << ConsoleColour() << "\".\n";
		}
	}

	SDL_JoystickEventState(SDL_QUERY);
	SDL_JoystickEventState(SDL_ENABLE);
}

void InputSDL::deInit()
{
	keyboards.clear();
	mice.clear();
	sdlJoys.clear();
	joysticks.clear();
}

std::shared_ptr<InputKeyboard> InputSDL::getKeyboard(int id) const
{
	return keyboards.at(id);
}

std::shared_ptr<InputJoystick> InputSDL::getJoystick(int id) const
{
	return joysticks.at(id);
}

std::shared_ptr<InputMouse> InputSDL::getMouse(int id) const
{
	return mice.at(id);
}

size_t InputSDL::getNumberOfKeyboards() const
{
	return keyboards.size();
}

size_t InputSDL::getNumberOfJoysticks() const
{
	return joysticks.size();
}

size_t InputSDL::getNumberOfMice() const
{
	return mice.size();
}

void InputSDL::beginEvents(Time t)
{
	// Keyboards
	size_t n = getNumberOfKeyboards();
	for (size_t i=0; i < n; i++) {
		keyboards[i]->clearPresses();
	}

	// Joysticks
	n = getNumberOfJoysticks();
	for (size_t i=0; i < n; i++) {
		auto& j = joysticks[i];
		j->clearPresses();
		j->update(t);
	}

	// Mice
	n = getNumberOfMice();
	for (size_t i=0; i < n; i++) {
		mice[i]->clearPresses();
		mice[i]->update();
	}

	// Touch events
	for (auto i=touchEvents.begin(); i!=touchEvents.end(); ) {
		auto next = i;
		++next;

		i->second->update(t);
		if (i->second->isReleased()) {
			touchEvents.erase(i);
		}

		i = next;
	}
}

void InputSDL::processEvent(SDL_Event& event)
{
	switch (event.type) {
		case SDL_KEYDOWN:
		case SDL_KEYUP:
		case SDL_TEXTINPUT:
		case SDL_TEXTEDITING:
			{
				size_t n = getNumberOfKeyboards();
				for (size_t i=0; i < n; i++) {
					keyboards[i]->processEvent(event);
				}
				break;
			}

		case SDL_JOYAXISMOTION:
			processJoyEvent(event.jaxis.which, event);
			break;
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
			processJoyEvent(event.jbutton.which, event);
			break;
		case SDL_JOYHATMOTION:
			processJoyEvent(event.jhat.which, event);
			break;
		case SDL_JOYBALLMOTION:
			processJoyEvent(event.jball.which, event);
			break;

		case SDL_MOUSEMOTION:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEWHEEL:
			{
				size_t n = getNumberOfMice();
				for (size_t i=0; i < n; i++) {
					mice[i]->processEvent(event);
				}
				break;
			}

		case SDL_FINGERDOWN:
		case SDL_FINGERUP:
		case SDL_FINGERMOTION:
			processTouch(event.type, event.tfinger.touchId, event.tfinger.fingerId, event.tfinger.x, event.tfinger.y);
			break;

		default:
			break;
	}
}

void InputSDL::processJoyEvent(int n, SDL_Event& event)
{
	auto iter = sdlJoys.find(n);
	if (iter != sdlJoys.end()) {
		iter->second->processEvent(event);
	}
}

void Halley::InputSDL::processTouch(int type, long long /*touchDeviceId*/, long long fingerId, float x, float y)
{
	Vector2f windowSize = Vector2f(1, 1); // TODO
	Vector2f origin = Vector2f(0, 0); // TODO
	Vector2f scale = Vector2f(1, 1); // TODO
	Vector2f screenPos = Vector2f(x, y) * Vector2f(windowSize);
	Vector2f pos = Vector2f(Vector2i((screenPos - origin) / scale));

	spInputTouch touch;
	if (type == SDL_FINGERDOWN) {
		// New event
		touch = spInputTouch(new InputTouch(pos));
		touchEvents[static_cast<int>(fingerId)] = touch;
	} else {
		// Update existing
		auto i = touchEvents.find(static_cast<int>(fingerId));
		if (i == touchEvents.end()) throw Exception("Unknown touchId: "+toString(fingerId));
		touch = i->second;
		touch->setPos(pos);
		if (type == SDL_FINGERUP) {
			touch->setReleased();
		}
	}
}

Vector<spInputTouch> Halley::InputSDL::getNewTouchEvents()
{
	Vector<spInputTouch> result;
	for (auto i : touchEvents) {
		if (i.second->isPressed()) {
			result.push_back(i.second);
		}
	}
	return result;
}

Vector<spInputTouch> Halley::InputSDL::getTouchEvents()
{
	Vector<spInputTouch> result;
	for (auto i : touchEvents) {
		result.push_back(i.second);
	}
	return result;
}
