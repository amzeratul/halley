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

#include "input_game_controller_sdl.h"
#include "sdl_rw_ops.h"
#include "halley/file_formats/binary_file.h"
#include "halley/support/console.h"
#include "halley/text/string_converter.h"
#include "halley/utils/algorithm.h"

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

InputSDL::~InputSDL()
{
	for (auto& c: cursors) {
		SDL_FreeCursor(c.second);
	}
	cursors.clear();
}

void InputSDL::setResources(Resources& resources)
{
	std::cout << ConsoleColour(Console::GREEN) << "\nInitialized SDL input..." << ConsoleColour() << "\n";
	if (resources.exists<BinaryFile>("binary/sdl/gamecontrollerdb.txt")) {
		const auto db = resources.get<BinaryFile>("binary/sdl/gamecontrollerdb.txt");
		const auto bytes = db->getSpan();
		auto rw = SDL_RWFromConstMem(bytes.data(), int(bytes.size()));
		const int added = SDL_GameControllerAddMappingsFromRW(rw, 1);
		std::cout << "\tLoaded " << ConsoleColour(Console::DARK_GREY) << toString(added) << ConsoleColour() << " SDL controller mappings.\n";
	}

	mouseRemap = [] (Vector2i p) { return Vector2f(p); };

	keyboards.push_back(std::unique_ptr<InputKeyboardSDL>(new InputKeyboardSDL(system.getClipboard())));
	mice.push_back(std::unique_ptr<InputMouseSDL>(new InputMouseSDL()));

	// XInput controllers
#ifdef XINPUT_AVAILABLE
	for (int i = 0; i < 4; i++) {
		auto joy = std::make_shared<InputJoystickXInput>(i);
		joy->update(0);
		joysticks.push_back(std::move(joy));
	}
#endif

	SDL_JoystickEventState(SDL_QUERY);
	SDL_JoystickEventState(SDL_ENABLE);
}

void InputSDL::init()
{
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
	if (id < 0 || id >= static_cast<int>(keyboards.size())) {
		return {};
	}
	return keyboards[id];
}

std::shared_ptr<InputDevice> InputSDL::getJoystick(int id) const
{
	if (id < 0 || id >= static_cast<int>(joysticks.size())) {
		return {};
	}
	return joysticks[id];
}

std::shared_ptr<InputDevice> InputSDL::getMouse(int id) const
{
	if (id < 0 || id >= static_cast<int>(mice.size())) {
		return {};
	}
	return mice[id];
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
	for (auto i = touchEvents.begin(); i != touchEvents.end(); ) {
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
		case SDL_JOYDEVICEADDED:
		case SDL_JOYDEVICEREMOVED:
			processJoyDeviceEvent(event.jdevice);
			break;

		case SDL_CONTROLLERAXISMOTION:
			processGameControllerEvent(event.caxis.which, event);
			break;
		case SDL_CONTROLLERBUTTONDOWN:
		case SDL_CONTROLLERBUTTONUP:
			processGameControllerEvent(event.cbutton.which, event);
			break;
		case SDL_CONTROLLERDEVICEREMAPPED:
			break;
		case SDL_CONTROLLERDEVICEADDED:
		case SDL_CONTROLLERDEVICEREMOVED:
			processGameControllerDeviceEvent(event.cdevice);
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
		const auto iter = sdlJoys.find(event.which);
		if (iter != sdlJoys.end()) {
			iter->second->close();
			std_ex::erase(joysticks, iter->second);
			sdlJoys.erase(iter);
		}
	}
}

void InputSDL::processGameControllerEvent(int n, const SDL_Event& event)
{
	const auto iter = sdlGameControllers.find(n);
	if (iter != sdlGameControllers.end()) {
		iter->second->processEvent(event);
	}
}

void InputSDL::processGameControllerDeviceEvent(const SDL_ControllerDeviceEvent& event)
{
	// Handled by Joystick events
	if (event.type == SDL_CONTROLLERDEVICEADDED) {
		addJoystick(event.which);
	} else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
		const auto iter = sdlGameControllers.find(event.which);
		if (iter != sdlGameControllers.end()) {
			iter->second->close();
			std_ex::erase(joysticks, iter->second);
			sdlGameControllers.erase(iter);
		}
	}
}

void InputSDL::addJoystick(int idx)
{
	try {
		const auto nameLower = String(SDL_JoystickNameForIndex(idx)).asciiLower();
		const auto guid = SDL_JoystickGetDeviceGUID(idx);
		char buffer[64] = {};
#if SDL_VERSION_ATLEAST(2, 24, 0)
		SDL_GUIDToString(guid, buffer, 64);
#endif
		const bool isXinputController = String(buffer) == "xinput" || nameLower.contains("xbox") || nameLower.contains("xinput");

		if (!hasXInput || !isXinputController) {
			if (SDL_IsGameController(idx)) {
				const auto joy = std::make_shared<InputGameControllerSDL>(idx);
				const auto id = joy->getSDLJoystickId();
				assert(!sdlGameControllers.contains(id));
				assert(!sdlJoys.contains(id));
				sdlGameControllers[id] = joy;
				joysticks.push_back(joy);
			} else {
				const auto joy = std::make_shared<InputJoystickSDL>(idx);
				const auto id = joy->getSDLJoystickId();
				assert(!sdlGameControllers.contains(id));
				assert(!sdlJoys.contains(id));
				sdlJoys[id] = joy;
				joysticks.push_back(joy);
			}
		}
	} catch (const std::exception& e) {
		Logger::logException(e);
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

namespace {
	SDL_SystemCursor getSDLSystemCursor(MouseCursorMode mode)
	{
		switch (mode) {
			case MouseCursorMode::Arrow:
				return SDL_SYSTEM_CURSOR_ARROW;
			case MouseCursorMode::IBeam:
				return SDL_SYSTEM_CURSOR_IBEAM;
			case MouseCursorMode::Wait:
				return SDL_SYSTEM_CURSOR_WAIT;
			case MouseCursorMode::Crosshair:
				return SDL_SYSTEM_CURSOR_CROSSHAIR;
			case MouseCursorMode::WaitArrow:
				return SDL_SYSTEM_CURSOR_WAITARROW;
			case MouseCursorMode::SizeNWSE:
				return SDL_SYSTEM_CURSOR_SIZENWSE;
			case MouseCursorMode::SizeNESW:
				return SDL_SYSTEM_CURSOR_SIZENESW;
			case MouseCursorMode::SizeWE:
				return SDL_SYSTEM_CURSOR_SIZEWE;
			case MouseCursorMode::SizeNS:
				return SDL_SYSTEM_CURSOR_SIZENS;
			case MouseCursorMode::SizeAll:
				return SDL_SYSTEM_CURSOR_SIZEALL;
			case MouseCursorMode::No:
				return SDL_SYSTEM_CURSOR_NO;
			case MouseCursorMode::Hand:
				return SDL_SYSTEM_CURSOR_HAND;
		}
		return SDL_SYSTEM_CURSOR_ARROW;
	}
}

void InputSDL::setMouseCursorMode(std::optional<MouseCursorMode> mode)
{
	if (mode == curCursor) {
		return;
	}
	curCursor = mode;

	const auto cursorId = getSDLSystemCursor(mode.value_or(MouseCursorMode::Arrow));
	const auto iter = cursors.find(cursorId);
	if (iter != cursors.end()) {
		SDL_SetCursor(iter->second);
	}
	auto* cursor = SDL_CreateSystemCursor(cursorId);
	SDL_SetCursor(cursor);
	cursors[cursorId] = cursor;
}

void InputSDL::setMouseTrap(bool shouldBeTrapped)
{
	//isMouseTrapped = shouldBeTrapped;
	//SDL_SetWindowGrab(system.getWindow(0)->getSDLWindow(), shouldBeTrapped ? SDL_TRUE : SDL_FALSE);
	SDL_SetRelativeMouseMode(shouldBeTrapped ? SDL_TRUE : SDL_FALSE);
}
