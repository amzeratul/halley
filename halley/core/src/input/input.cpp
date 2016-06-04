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

#include "input/input.h"
#include "input/input_joystick_sdl.h"
#include "input/input_mouse_concrete.h"
#include "input/input_keyboard_concrete.h"
#include "input/input_touch.h"
#include <SDL.h>

#ifdef _MSC_VER
#include "input/input_joystick_xinput.h"
#endif

using namespace Halley;

Input::Input() = default;
Input::~Input() = default;

void Input::init()
{
	keyboards.push_back(std::unique_ptr<InputKeyboardConcrete>(new InputKeyboardConcrete()));
	mice.push_back(std::unique_ptr<InputMouseConcrete>(new InputMouseConcrete()));

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
		// Don't add if it's a 360 controller
		if (!hasXInput || name.asciiLower().find("xbox 360") == String::npos) {
			sdlJoys[i] = joy.get();
			joysticks.push_back(std::move(joy));
		}
	}

	SDL_JoystickEventState(SDL_QUERY);
	SDL_JoystickEventState(SDL_ENABLE);
}

void Input::deInit()
{
	keyboards.clear();
	mice.clear();
	sdlJoys.clear();
	joysticks.clear();
}

InputKeyboard& Input::getKeyboard(int id) const
{
	return *keyboards.at(id);
}

InputJoystick& Input::getJoystick(int id) const
{
	return *joysticks.at(id);
}

InputMouse& Input::getMouse(int id) const
{
	return *mice.at(id);
}

size_t Input::getNumberOfKeyboards() const
{
	return keyboards.size();
}

size_t Input::getNumberOfJoysticks() const
{
	return joysticks.size();
}

size_t Input::getNumberOfMice() const
{
	return mice.size();
}

void Input::beginEvents(Time t)
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

void Input::processEvent(SDL_Event& event)
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

void Input::processJoyEvent(int n, SDL_Event& event)
{
	auto iter = sdlJoys.find(n);
	if (iter != sdlJoys.end()) {
		iter->second->processEvent(event);
	}
}

void Halley::Input::processTouch(int type, long long /*touchDeviceId*/, long long fingerId, float x, float y)
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
		if (i == touchEvents.end()) throw Exception("Unknown touchId: "+String::integerToString(static_cast<int>(fingerId)));
		touch = i->second;
		touch->setPos(pos);
		if (type == SDL_FINGERUP) {
			touch->setReleased();
		}
	}
}

Vector<spInputTouch> Halley::Input::getNewTouchEvents()
{
	Vector<spInputTouch> result;
	for (auto i : touchEvents) {
		if (i.second->isPressed()) {
			result.push_back(i.second);
		}
	}
	return result;
}

Vector<spInputTouch> Halley::Input::getTouchEvents()
{
	Vector<spInputTouch> result;
	for (auto i : touchEvents) {
		result.push_back(i.second);
	}
	return result;
}
