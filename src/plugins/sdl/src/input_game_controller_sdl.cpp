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
	char buffer[64];
	SDL_GUIDToString(SDL_JoystickGetDeviceGUID(number), buffer, 64);
	name = String(SDL_GameControllerName(controller)) + " : " + buffer;

	// Axes
	axes.resize(SDL_CONTROLLER_AXIS_MAX);
	axisAdjust = &defaultAxisAdjust;

	// Buttons
	init(SDL_CONTROLLER_BUTTON_MAX + 2);

	setEnabled(true);

	std::cout << "\tInitialized SDL Game Controller: \"" << ConsoleColour(Console::DARK_GREY) << getName() << " (idx = " << idx << ")" << ConsoleColour() << "\".\n";
	std::cout << "\t* Mapping: \"" << ConsoleColour(Console::DARK_GREY) << getMapping() << ConsoleColour() << "\".\n";
}

InputGameControllerSDL::~InputGameControllerSDL()
{
	close();
}

void InputGameControllerSDL::close()
{
	if (controller) {
		std::cout << "\tRemoved SDL Game Controller: \"" << ConsoleColour(Console::DARK_GREY) << getName() << " (idx = " << idx << ")" << ConsoleColour() << "\".\n";
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

int InputGameControllerSDL::getSDLJoystickId() const
{
	return id;
}

int InputGameControllerSDL::getButtonAtPosition(JoystickButtonPosition position) const
{
	switch (position) {
		case JoystickButtonPosition::FaceTop: return SDL_CONTROLLER_BUTTON_Y;
		case JoystickButtonPosition::FaceRight: return SDL_CONTROLLER_BUTTON_B;
		case JoystickButtonPosition::FaceBottom: return SDL_CONTROLLER_BUTTON_A;
		case JoystickButtonPosition::FaceLeft: return SDL_CONTROLLER_BUTTON_X;
		case JoystickButtonPosition::Select: return SDL_CONTROLLER_BUTTON_BACK;
		case JoystickButtonPosition::Start: return SDL_CONTROLLER_BUTTON_START;
		case JoystickButtonPosition::BumperLeft: return SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
		case JoystickButtonPosition::BumperRight: return SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
		case JoystickButtonPosition::TriggerLeft: return SDL_CONTROLLER_BUTTON_MAX;
		case JoystickButtonPosition::TriggerRight: return SDL_CONTROLLER_BUTTON_MAX + 1;
		case JoystickButtonPosition::LeftStick: return SDL_CONTROLLER_BUTTON_LEFTSTICK;
		case JoystickButtonPosition::RightStick: return SDL_CONTROLLER_BUTTON_RIGHTSTICK;
		case JoystickButtonPosition::PlatformAcceptButton: return SDL_CONTROLLER_BUTTON_A;
		case JoystickButtonPosition::PlatformCancelButton: return SDL_CONTROLLER_BUTTON_B;
		case JoystickButtonPosition::DPadUp: return SDL_CONTROLLER_BUTTON_DPAD_UP;
		case JoystickButtonPosition::DPadRight: return SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
		case JoystickButtonPosition::DPadDown: return SDL_CONTROLLER_BUTTON_DPAD_DOWN;
		case JoystickButtonPosition::DPadLeft: return SDL_CONTROLLER_BUTTON_DPAD_LEFT;
		case JoystickButtonPosition::System: return SDL_CONTROLLER_BUTTON_GUIDE;
		default: throw Exception("Invalid parameter", HalleyExceptions::InputPlugin);
	}
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
