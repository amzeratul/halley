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
#include "system_sdl.h"
#include "sdl_window.h"
#include "halley/input/input_touch.h"
#include <SDL.h>
#include "halley/support/console.h"
#include "halley/text/string_converter.h"

#ifdef _MSC_VER
#include "halley/input/input_joystick_xinput.h"
#endif

using namespace Halley;

#ifdef XINPUT_AVAILABLE
constexpr static bool hasXInput = true;
#else
constexpr static bool hasXInput = false;
#endif

InputSDL::InputSDL(SystemAPI& system)
	: system(dynamic_cast<SystemSDL&>(system))
{
}

InputSDL::~InputSDL() = default;

void InputSDL::init()
{
	mouseRemap = [] (Vector2i p) { return Vector2f(p); };

	keyboards.push_back(std::unique_ptr<InputKeyboardSDL>(new InputKeyboardSDL(system.getClipboard())));
	mice.push_back(std::unique_ptr<InputMouseSDL>(new InputMouseSDL()));

	// XInput controllers
#ifdef XINPUT_AVAILABLE
	for (int i = 0; i < 4; i++) {
		auto joy = std::make_unique<InputJoystickXInput>(i);
		joy->update(0);
		joysticks.push_back(std::move(joy));
	}
#endif

	// SDL joysticks
	const int nJoy = SDL_NumJoysticks();
	for (int i = 0; i < nJoy; i++) {
		addJoystick(i);
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

std::shared_ptr<InputDevice> InputSDL::getMouse(int id) const
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
	for (auto& keyboard: keyboards) {
		keyboard->update();
	}

	// Joysticks
	for (auto& joystick: joysticks) {
		joystick->update(t);
	}

	// Mice
	for (auto& mouse: mice) {
		mouse->update();

		mouse->setMouseTrapped(isMouseTrapped); // HACK can be removed once we update SDL
		if (isMouseTrapped) {
			const auto pos = Vector2i(system.getWindow(0)->getWindowRect().getWidth() / 2, system.getWindow(0)->getWindowRect().getHeight() / 2);
			setMouseCursorPos(pos);
			mouse->setDeltaPos(pos);
		}
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
			for (auto& keyboard: keyboards) {
				keyboard->processEvent(event);
			}
			break;

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
			for (auto& mouse: mice) {
				mouse->processEvent(event, mouseRemap);
			}
			break;

		case SDL_FINGERDOWN:
		case SDL_FINGERUP:
		case SDL_FINGERMOTION:
			processTouch(event.type, event.tfinger.touchId, event.tfinger.fingerId, event.tfinger.x, event.tfinger.y);
			break;

		case SDL_JOYDEVICEADDED:
		case SDL_JOYDEVICEREMOVED:
			processJoyDeviceEvent(event.jdevice);
			break;

		default:
			break;
	}
}

void InputSDL::setMouseRemapping(std::function<Vector2f(Vector2i)> remapFunction)
{
	mouseRemap = remapFunction;
	for (auto& mouse: mice) {
		mouse->updateRemap(mouseRemap);
	}
}

void InputSDL::processJoyEvent(int n, const SDL_Event& event)
{
	const auto iter = sdlJoys.find(n);
	if (iter != sdlJoys.end()) {
		iter->second->processEvent(event);
	}
}

void InputSDL::processJoyDeviceEvent(const SDL_JoyDeviceEvent& event)
{
	if (event.type == SDL_JOYDEVICEADDED) {
		addJoystick(event.which);
	} else if (event.type == SDL_JOYDEVICEREMOVED) {
		sdlJoys[event.which]->close();
		sdlJoys.erase(event.which);
	}
}

void InputSDL::addJoystick(int idx)
{
	auto joy = std::unique_ptr<InputJoystickSDL>(new InputJoystickSDL(idx));

	const String& name = joy->getName();
	const bool isXinputController = name.asciiLower().find("xbox 360") != String::npos || name.asciiLower().find("xinput") != String::npos;

	if (!hasXInput || !isXinputController) {
		const auto id = joy->getSDLJoystickId();
		sdlJoys[id] = joy.get();
		joysticks.push_back(std::move(joy));

		std::cout << "\tInitialized SDL joystick: \"" << ConsoleColour(Console::DARK_GREY) << name << ConsoleColour() << "\".\n";
	}
}

void InputSDL::processTouch(int type, long long /*touchDeviceId*/, long long fingerId, float x, float y)
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
		if (i == touchEvents.end()) throw Exception("Unknown touchId: " + toString(fingerId), HalleyExceptions::InputPlugin);
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
	for (const auto& i : touchEvents) {
		if (i.second->isPressed()) {
			result.push_back(i.second);
		}
	}
	return result;
}

Vector<spInputTouch> Halley::InputSDL::getTouchEvents()
{
	Vector<spInputTouch> result;
	for (const auto& i : touchEvents) {
		result.push_back(i.second);
	}
	return result;
}

void InputSDL::setMouseCursorPos(Vector2i pos)
{
	SDL_WarpMouseInWindow(system.getWindow(0)->getSDLWindow(), pos.x, pos.y);
}

void InputSDL::setMouseTrap(bool shouldBeTrapped)
{
	isMouseTrapped = shouldBeTrapped;
	SDL_SetWindowGrab(system.getWindow(0)->getSDLWindow(), shouldBeTrapped ? SDL_TRUE : SDL_FALSE);
}
