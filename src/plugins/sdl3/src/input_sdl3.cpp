#include "input_sdl3.h"
#include "input_joystick_sdl3.h"
#include "input_mouse_sdl3.h"
#include "input_keyboard_sdl3.h"
#include "system_sdl3.h"
#include "sdl3_window.h"
#include "halley/input/input_touch.h"

#include "input_game_controller_sdl3.h"
#include "sdl3_rw_ops.h"
#include "halley/file_formats/binary_file.h"
#include "halley/support/console.h"
#include "halley/text/string_converter.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

InputSDL3::InputSDL3(SystemAPI& system)
	: system(dynamic_cast<SystemSDL3&>(system))
{
}

InputSDL3::~InputSDL3()
{
	for (auto& c: cursors) {
        SDL_DestroyCursor(c.second);
	}
	cursors.clear();
}

void InputSDL3::setResources(Resources& resources)
{
#if defined(_GAMING_DESKTOP)
	const char* dbName = "binary/sdl/gamecontrollerdb_gdk.txt";
#elif defined(_GAMING_XBOX_SCARLETT)
	const char* dbName = "binary/sdl/gamecontrollerdb_scarlett.txt";
#elif defined(_GAMING_XBOX_XBOXONE)
	const char* dbName = "binary/sdl/gamecontrollerdb_xboxone.txt";
#else
	const char* dbName = "binary/sdl/gamecontrollerdb.txt";
#endif

	Logger::logInfo("Initialized SDL input...");
	if (resources.exists<BinaryFile>(dbName)) {
		const auto db = resources.get<BinaryFile>(dbName);
		const auto bytes = db->getSpan();
		auto rw = SDL_IOFromConstMem(bytes.data(), bytes.size());
		const int added = SDL_AddGamepadMappingsFromIO(rw, true);
		Logger::logInfo("\tLoaded " + toString(added) + " SDL controller mappings.");
	} else {
		Logger::logWarning("\tNo SDL game controller mapping file found.");
	}

	mouseRemap = [] (Vector2i p) { return Vector2f(p); };

	keyboards.push_back(std::unique_ptr<InputKeyboardSDL3>(new InputKeyboardSDL3(system, system.getClipboard())));
	mice.push_back(std::unique_ptr<InputMouseSDL3>(new InputMouseSDL3()));

    SDL_SetJoystickEventsEnabled(true);

	int numGamepads = 0;
	SDL_JoystickID* gamepads = SDL_GetGamepads(&numGamepads);
	if (gamepads != nullptr) {
		for (int i = 0; i < numGamepads; i++) {
			addJoystick(gamepads[i]);
		}
		SDL_free(gamepads);
	}
}

void InputSDL3::init()
{
}

void InputSDL3::deInit()
{
	keyboards.clear();
	mice.clear();
	sdlJoys.clear();
	joysticks.clear();
}

std::shared_ptr<InputKeyboard> InputSDL3::getKeyboard(int id) const
{
	if (id < 0 || id >= static_cast<int>(keyboards.size())) {
		return {};
	}
	return keyboards[id];
}

std::shared_ptr<InputDevice> InputSDL3::getJoystick(int id) const
{
	if (id < 0 || id >= static_cast<int>(joysticks.size())) {
		return {};
	}
	return joysticks[id];
}

std::shared_ptr<InputDevice> InputSDL3::getMouse(int id) const
{
	if (id < 0 || id >= static_cast<int>(mice.size())) {
		return {};
	}
	return mice[id];
}

size_t InputSDL3::getNumberOfKeyboards() const
{
	return keyboards.size();
}

size_t InputSDL3::getNumberOfJoysticks() const
{
	return joysticks.size();
}

size_t InputSDL3::getNumberOfMice() const
{
	return mice.size();
}

void InputSDL3::beginEvents(Time t)
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

void InputSDL3::processEvent(SDL_Event& event)
{
	switch (event.type) {
		case SDL_EVENT_KEY_DOWN:
		case SDL_EVENT_KEY_UP:
		case SDL_EVENT_TEXT_INPUT:
		case SDL_EVENT_TEXT_EDITING:
			for (auto& keyboard: keyboards) {
				keyboard->processEvent(event);
			}
			break;

		case SDL_EVENT_JOYSTICK_AXIS_MOTION:
			processJoyEvent(event.jaxis.which, event);
			break;
		case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
		case SDL_EVENT_JOYSTICK_BUTTON_UP:
			processJoyEvent(event.jbutton.which, event);
			break;
		case SDL_EVENT_JOYSTICK_HAT_MOTION:
			processJoyEvent(event.jhat.which, event);
			break;
		case SDL_EVENT_JOYSTICK_BALL_MOTION:
			processJoyEvent(event.jball.which, event);
			break;
		case SDL_EVENT_JOYSTICK_ADDED:
		case SDL_EVENT_JOYSTICK_REMOVED:
			processJoyDeviceEvent(event.jdevice);
			break;

		case SDL_EVENT_GAMEPAD_AXIS_MOTION:
			processGameControllerEvent(event.gaxis.which, event);
			break;
		case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
		case SDL_EVENT_GAMEPAD_BUTTON_UP:
			processGameControllerEvent(event.gbutton.which, event);
			break;
		case SDL_EVENT_GAMEPAD_REMAPPED:
			break;
		case SDL_EVENT_GAMEPAD_ADDED:
		case SDL_EVENT_GAMEPAD_REMOVED:
			processGameControllerDeviceEvent(event.gdevice);
			break;

		case SDL_EVENT_MOUSE_MOTION:
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		case SDL_EVENT_MOUSE_BUTTON_UP:
		case SDL_EVENT_MOUSE_WHEEL:
			for (auto& mouse: mice) {
				mouse->processEvent(event, mouseRemap);
			}
			break;

		case SDL_EVENT_FINGER_DOWN:
		case SDL_EVENT_FINGER_UP:
		case SDL_EVENT_FINGER_MOTION:
			processTouch(event.type, event.tfinger.touchID, event.tfinger.fingerID, event.tfinger.x, event.tfinger.y);
			break;

		default:
			break;
	}
}

void InputSDL3::setMouseRemapping(std::function<Vector2f(Vector2i)> remapFunction)
{
	mouseRemap = remapFunction;
	for (auto& mouse: mice) {
		mouse->updateRemap(mouseRemap);
	}
}

void InputSDL3::processJoyEvent(int n, const SDL_Event& event)
{
	const auto iter = sdlJoys.find(n);
	if (iter != sdlJoys.end()) {
		iter->second->processEvent(event);
	}
}

void InputSDL3::processJoyDeviceEvent(const SDL_JoyDeviceEvent& event)
{
	if (event.type == SDL_EVENT_JOYSTICK_ADDED) {
		addJoystick(event.which);
	} else if (event.type == SDL_EVENT_JOYSTICK_REMOVED) {
		const auto iter = sdlJoys.find(event.which);
		if (iter != sdlJoys.end()) {
			iter->second->close();
			std_ex::erase(joysticks, iter->second);
			sdlJoys.erase(iter);
		}
	}
}

void InputSDL3::processGameControllerEvent(int n, const SDL_Event& event)
{
	const auto iter = sdlGameControllers.find(n);
	if (iter != sdlGameControllers.end()) {
		iter->second->processEvent(event);
	}
}

void InputSDL3::processGameControllerDeviceEvent(const SDL_GamepadDeviceEvent& event)
{
	// Handled by Joystick events
	if (event.type == SDL_EVENT_GAMEPAD_ADDED) {
		addJoystick(event.which);
	} else if (event.type == SDL_EVENT_GAMEPAD_REMOVED) {
		const auto iter = sdlGameControllers.find(event.which);
		if (iter != sdlGameControllers.end()) {
			iter->second->close();
			std_ex::erase(joysticks, iter->second);
			sdlGameControllers.erase(iter);
		}
	}
}

void InputSDL3::addJoystick(SDL_JoystickID instanceId)
{
	if (SDL_IsGamepad(instanceId)) {
		if (!sdlGameControllers.contains(instanceId)) {
			const auto joy = std::make_shared<InputGameControllerSDL3>(instanceId);
			assert(!sdlJoys.contains(instanceId));
			sdlGameControllers[instanceId] = joy;
			joysticks.push_back(joy);
		}
	} else {
		if (!sdlJoys.contains(instanceId)) {
			const auto joy = std::make_shared<InputJoystickSDL3>(instanceId);
			assert(!sdlGameControllers.contains(instanceId));
			sdlJoys[instanceId] = joy;
			joysticks.push_back(joy);
		}
	}
}

void InputSDL3::processTouch(int type, long long /*touchDeviceId*/, long long fingerId, float x, float y)
{
	Vector2f windowSize = Vector2f(1, 1); // TODO
	Vector2f origin = Vector2f(0, 0); // TODO
	Vector2f scale = Vector2f(1, 1); // TODO
	Vector2f screenPos = Vector2f(x, y) * Vector2f(windowSize);
	Vector2f pos = Vector2f(Vector2i((screenPos - origin) / scale));

	spInputTouch touch;
	if (type == SDL_EVENT_FINGER_DOWN) {
		// New event
		touch = spInputTouch(new InputTouch(pos));
		touchEvents[static_cast<int>(fingerId)] = touch;
	} else {
		// Update existing
		auto i = touchEvents.find(static_cast<int>(fingerId));
		if (i == touchEvents.end()) throw Exception("Unknown touchId: " + toString(fingerId), HalleyExceptions::InputPlugin);
		touch = i->second;
		touch->setPos(pos);
		if (type == SDL_EVENT_FINGER_UP) {
			touch->setReleased();
		}
	}
}

Vector<spInputTouch> InputSDL3::getNewTouchEvents()
{
	Vector<spInputTouch> result;
	for (const auto& i : touchEvents) {
		if (i.second->isPressed()) {
			result.push_back(i.second);
		}
	}
	return result;
}

Vector<spInputTouch> InputSDL3::getTouchEvents()
{
	Vector<spInputTouch> result;
	for (const auto& i : touchEvents) {
		result.push_back(i.second);
	}
	return result;
}

void InputSDL3::setMouseCursorPos(Vector2i pos)
{
	SDL_WarpMouseInWindow(system.getWindow(0)->getSDLWindow(), (float) pos.x, (float) pos.y);
}

namespace {
	SDL_SystemCursor getSDLSystemCursor(MouseCursorMode mode)
	{
		switch (mode) {
			case MouseCursorMode::Arrow:
				return SDL_SYSTEM_CURSOR_DEFAULT;
			case MouseCursorMode::IBeam:
				return SDL_SYSTEM_CURSOR_TEXT;
			case MouseCursorMode::Wait:
				return SDL_SYSTEM_CURSOR_WAIT;
			case MouseCursorMode::Crosshair:
				return SDL_SYSTEM_CURSOR_CROSSHAIR;
			case MouseCursorMode::WaitArrow:
				return SDL_SYSTEM_CURSOR_PROGRESS;
			case MouseCursorMode::SizeNWSE:
				return SDL_SYSTEM_CURSOR_NWSE_RESIZE;
			case MouseCursorMode::SizeNESW:
				return SDL_SYSTEM_CURSOR_NESW_RESIZE;
			case MouseCursorMode::SizeWE:
				return SDL_SYSTEM_CURSOR_EW_RESIZE;
			case MouseCursorMode::SizeNS:
				return SDL_SYSTEM_CURSOR_NS_RESIZE;
			case MouseCursorMode::SizeAll:
				return SDL_SYSTEM_CURSOR_MOVE;
			case MouseCursorMode::No:
				return SDL_SYSTEM_CURSOR_NOT_ALLOWED;
			case MouseCursorMode::Hand:
				return SDL_SYSTEM_CURSOR_POINTER;
		}
		return SDL_SYSTEM_CURSOR_DEFAULT;
	}
}

void InputSDL3::setMouseCursorMode(std::optional<MouseCursorMode> mode)
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

void InputSDL3::setMouseTrap(bool shouldBeTrapped)
{
	//isMouseTrapped = shouldBeTrapped;
	//SDL_SetWindowGrab(system.getWindow(0)->getSDLWindow(), shouldBeTrapped ? SDL_TRUE : SDL_FALSE);
	SDL_SetWindowRelativeMouseMode(system.getWindow(0)->getSDLWindow(), shouldBeTrapped);
}
