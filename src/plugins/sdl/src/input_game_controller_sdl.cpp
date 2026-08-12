#include "input_game_controller_sdl.h"

#include <SDL.h>
#include <SDL_gamecontroller.h>

#include "halley/support/console.h"

using namespace Halley;

InputGameControllerSDL::InputGameControllerSDL(int number)
{
	controller = SDL_GameControllerOpen(number);
	if (!controller) {
		throw Exception("Could not open Game Controller " + toString(number) + ": " + toString(SDL_GetError()), HalleyExceptions::InputPlugin);
	}
	id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller));
	idx = number;
	name = String(SDL_GameControllerName(controller));
	const auto type = SDL_GameControllerGetType(controller);

	// Category
	if (type == SDL_CONTROLLER_TYPE_XBOX360 || type == SDL_CONTROLLER_TYPE_XBOXONE) {
		joystickType = JoystickType::Xbox;
	} else if (type == SDL_CONTROLLER_TYPE_PS3 || type == SDL_CONTROLLER_TYPE_PS4 || type == SDL_CONTROLLER_TYPE_PS5) {
		joystickType = JoystickType::Playstation;
	} else if (type == SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO || type == SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR) {
		joystickType = JoystickType::SwitchFull;
	} else if (type == SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_LEFT) {
		joystickType = JoystickType::SwitchLeftJoycon;
	} else if (type == SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT) {
		joystickType = JoystickType::SwitchRightJoycon;
	} else {
		auto nameLower = name.asciiLower();
		if (nameLower.contains("xinput") || nameLower.contains("xbox")) {
			joystickType = JoystickType::Xbox;
		} else if (nameLower.contains("playstation") || nameLower.contains("dualshock") || nameLower.contains("dualsense")) {
			joystickType = JoystickType::Playstation;
		} else if (nameLower.contains("nintendo") || nameLower.contains("switch")) {
			joystickType = JoystickType::SwitchFull;
		} else {
			joystickType = JoystickType::Generic;
		}
	}

	// Axes
	axes.resize(SDL_CONTROLLER_AXIS_MAX);
	axisAdjust = &defaultAxisAdjust;

	// Buttons
	init(SDL_CONTROLLER_BUTTON_MAX + 2);

	setEnabled(true);

	Logger::logInfo(String("\tInitialized SDL Game Controller: \"") + getName() + " (idx = " + idx + ")\".");
	Logger::logInfo("\t* Mapping: \"" + getMapping() + "\".");
}

InputGameControllerSDL::~InputGameControllerSDL()
{
	close();
}

void InputGameControllerSDL::close()
{
	if (controller) {
		Logger::logInfo(String("\tRemoved SDL Game Controller: \"") + getName() + " (idx = " + idx + ")\".");
		SDL_GameControllerClose(controller);
		controller = nullptr;
		id = -1;
		idx = -1;
		setEnabled(false);
	}
}

void InputGameControllerSDL::update(Time t)
{
	clearPresses();
	InputJoystick::update(t);
}

std::string_view InputGameControllerSDL::getName() const
{
	return name;
}

String InputGameControllerSDL::getMapping() const
{
	return controller ? SDL_GameControllerMapping(controller) : nullptr;
}

JoystickType InputGameControllerSDL::getJoystickType() const
{
	return joystickType;
}

int InputGameControllerSDL::getSDLJoystickId() const
{
	return id;
}

int InputGameControllerSDL::getButtonAtPosition(JoystickButtonPosition position) const
{
	const bool nintendo = joystickType == JoystickType::SwitchFull || joystickType == JoystickType::SwitchLeftJoycon || joystickType == JoystickType::SwitchRightJoycon;
	const bool playstation = joystickType == JoystickType::Playstation;

	switch (position) {
	case JoystickButtonPosition::FaceTop:
		return nintendo ? SDL_CONTROLLER_BUTTON_X : SDL_CONTROLLER_BUTTON_Y;
	case JoystickButtonPosition::FaceRight:
		return nintendo ? SDL_CONTROLLER_BUTTON_A : SDL_CONTROLLER_BUTTON_B;
	case JoystickButtonPosition::FaceBottom:
		return nintendo ? SDL_CONTROLLER_BUTTON_B : SDL_CONTROLLER_BUTTON_A;
	case JoystickButtonPosition::FaceLeft:
		return nintendo ? SDL_CONTROLLER_BUTTON_Y : SDL_CONTROLLER_BUTTON_X;
	case JoystickButtonPosition::Select:
		return playstation ? SDL_CONTROLLER_BUTTON_TOUCHPAD : SDL_CONTROLLER_BUTTON_BACK;
	case JoystickButtonPosition::Start:
		return SDL_CONTROLLER_BUTTON_START;
	case JoystickButtonPosition::BumperLeft:
		return SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
	case JoystickButtonPosition::BumperRight:
		return SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
	case JoystickButtonPosition::TriggerLeft:
		return SDL_CONTROLLER_BUTTON_MAX;
	case JoystickButtonPosition::TriggerRight:
		return SDL_CONTROLLER_BUTTON_MAX + 1;
	case JoystickButtonPosition::LeftStick:
		return SDL_CONTROLLER_BUTTON_LEFTSTICK;
	case JoystickButtonPosition::RightStick:
		return SDL_CONTROLLER_BUTTON_RIGHTSTICK;
	case JoystickButtonPosition::Accept:
		return SDL_CONTROLLER_BUTTON_A;
	case JoystickButtonPosition::Cancel:
		return SDL_CONTROLLER_BUTTON_B;
	case JoystickButtonPosition::DPadUp:
		return SDL_CONTROLLER_BUTTON_DPAD_UP;
	case JoystickButtonPosition::DPadRight:
		return SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
	case JoystickButtonPosition::DPadDown:
		return SDL_CONTROLLER_BUTTON_DPAD_DOWN;
	case JoystickButtonPosition::DPadLeft:
		return SDL_CONTROLLER_BUTTON_DPAD_LEFT;
	case JoystickButtonPosition::System:
		return SDL_CONTROLLER_BUTTON_GUIDE;
	case JoystickButtonPosition::Misc1:
		return SDL_CONTROLLER_BUTTON_MISC1;
	case JoystickButtonPosition::Paddle1:
		return SDL_CONTROLLER_BUTTON_PADDLE1;
	case JoystickButtonPosition::Paddle2:
		return SDL_CONTROLLER_BUTTON_PADDLE2;
	case JoystickButtonPosition::Paddle3:
		return SDL_CONTROLLER_BUTTON_PADDLE3;
	case JoystickButtonPosition::Paddle4:
		return SDL_CONTROLLER_BUTTON_PADDLE4;
	case JoystickButtonPosition::TouchPad:
		return SDL_CONTROLLER_BUTTON_TOUCHPAD;
	default: 
		throw Exception("Invalid parameter", HalleyExceptions::InputPlugin);
	}
}

std::optional<JoystickButtonPosition> InputGameControllerSDL::getPositionForButton(int code) const
{
	const bool nintendo = joystickType == JoystickType::SwitchFull || joystickType == JoystickType::SwitchLeftJoycon || joystickType == JoystickType::SwitchRightJoycon;
	const bool playstation = joystickType == JoystickType::Playstation;

	using enum JoystickButtonPosition;

	switch (code) {
	case SDL_CONTROLLER_BUTTON_A:
		return nintendo ? FaceRight : FaceBottom;
    case SDL_CONTROLLER_BUTTON_B:
		return nintendo ? FaceBottom : FaceRight;
    case SDL_CONTROLLER_BUTTON_X:
		return nintendo ? FaceTop : FaceLeft;
    case SDL_CONTROLLER_BUTTON_Y:
		return nintendo ? FaceLeft : FaceTop;
    case SDL_CONTROLLER_BUTTON_BACK:
		return Select;
    case SDL_CONTROLLER_BUTTON_GUIDE:
		return System;
    case SDL_CONTROLLER_BUTTON_START:
		return Start;
    case SDL_CONTROLLER_BUTTON_LEFTSTICK:
		return LeftStick;
    case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
		return RightStick;
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
		return BumperLeft;
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
		return BumperRight;
    case SDL_CONTROLLER_BUTTON_DPAD_UP:
		return DPadUp;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
		return DPadDown;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
		return DPadLeft;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
		return DPadRight;
	case SDL_CONTROLLER_BUTTON_MISC1:
		return Misc1;
    case SDL_CONTROLLER_BUTTON_PADDLE1:
		return Paddle1;
    case SDL_CONTROLLER_BUTTON_PADDLE2:
		return Paddle2;
    case SDL_CONTROLLER_BUTTON_PADDLE3:
		return Paddle3;
    case SDL_CONTROLLER_BUTTON_PADDLE4:
		return Paddle4;
	case SDL_CONTROLLER_BUTTON_TOUCHPAD:
		return playstation ? Select : TouchPad;
	case SDL_CONTROLLER_BUTTON_MAX:
		return TriggerLeft;
	case SDL_CONTROLLER_BUTTON_MAX + 1:
		return TriggerRight;
	default: 
		throw Exception("Unknown button for SDL controller: " + toString(code), HalleyExceptions::InputPlugin);
	}
}

bool InputGameControllerSDL::hasLED() const
{
	return SDL_GameControllerHasLED(controller);
}

void InputGameControllerSDL::setLED(Colour4c c) const
{
	SDL_GameControllerSetLED(controller, c.r, c.g, c.b);
}

std::optional<int> InputGameControllerSDL::getPlayerIndex() const
{
	if (auto idx = SDL_GameControllerGetPlayerIndex(controller); idx != -1) {
		return idx;
	} else {
		return {};
	}
}

int InputGameControllerSDL::getControllerIndex() const
{
	return idx;
}

void InputGameControllerSDL::processEvent(const SDL_Event& event)
{
	if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) {
		const bool down = event.type == SDL_CONTROLLERBUTTONDOWN;
		onButtonStatus(event.cbutton.button, down);
	} else if (event.type == SDL_CONTROLLERAXISMOTION) {
		const auto id = event.caxis.axis;
		const auto value = static_cast<float>(event.caxis.value) / static_cast<float>(std::numeric_limits<int16_t>::max());
		axes[id] = value;
		if (id == SDL_CONTROLLER_AXIS_TRIGGERLEFT) {
			onButtonStatus(SDL_CONTROLLER_BUTTON_MAX, value > 0.5f);
		} else if (id == SDL_CONTROLLER_AXIS_TRIGGERRIGHT) {
			onButtonStatus(SDL_CONTROLLER_BUTTON_MAX + 1, value > 0.5f);
		}
	}
}

void InputGameControllerSDL::doSetVibration(float low, float high)
{
	if (controller) {
		SDL_GameControllerRumble(controller, static_cast<uint16_t>(low * 65535), static_cast<uint16_t>(high * 65535), 100);
	}
}
