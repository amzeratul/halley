#include "input_game_controller_sdl3.h"

#include "halley/support/console.h"

using namespace Halley;

InputGameControllerSDL3::InputGameControllerSDL3(SDL_JoystickID instanceId)
	: id(instanceId)
{
	controller = SDL_OpenGamepad(instanceId);
	if (!controller) {
		throw Exception("Could not open Game Controller " + toString(instanceId) + ": " + toString(SDL_GetError()), HalleyExceptions::InputPlugin);
	}
	name = String(SDL_GetGamepadName(controller));

	// Axes
	axes.resize(SDL_GAMEPAD_AXIS_COUNT);
	axisAdjust = &defaultAxisAdjust;

	// Hat
	hats.resize(1);
	hats[0] = std::make_shared<InputButtonBase>(4);

	// Buttons
	init(SDL_GAMEPAD_BUTTON_COUNT + 2);

	setEnabled(true);

	std::cout << "\tInitialized SDL Game Controller: \"" << ConsoleColour(Console::DARK_GREY) << getName() << " (id = " << instanceId << ")" << ConsoleColour() << "\".\n";
	std::cout << "\t* Mapping: \"" << ConsoleColour(Console::DARK_GREY) << getMapping() << ConsoleColour() << "\".\n";
}

InputGameControllerSDL3::~InputGameControllerSDL3()
{
	close();
}

void InputGameControllerSDL3::close()
{
	if (controller) {
		std::cout << "\tRemoved SDL Game Controller: \"" << ConsoleColour(Console::DARK_GREY) << getName() << " (idx = " << id << ")" << ConsoleColour() << "\".\n";
		doSetVibration(0, 0);
		SDL_CloseGamepad(controller);
		controller = nullptr;
		id = ~0;
		setEnabled(false);
	}
}

void InputGameControllerSDL3::update(Time t)
{
	clearPresses();
	InputJoystick::update(t);
}

std::string_view InputGameControllerSDL3::getName() const
{
	return name;
}

String InputGameControllerSDL3::getMapping() const
{
	return controller ? SDL_GetGamepadMapping(controller) : nullptr;
}

SDL_JoystickID InputGameControllerSDL3::getSDLJoystickId() const
{
	return id;
}

int InputGameControllerSDL3::getButtonAtPosition(JoystickButtonPosition position) const
{
	switch (position) {
		case JoystickButtonPosition::FaceTop: return SDL_GAMEPAD_BUTTON_NORTH;
		case JoystickButtonPosition::FaceRight: return SDL_GAMEPAD_BUTTON_EAST;
		case JoystickButtonPosition::FaceBottom: return SDL_GAMEPAD_BUTTON_SOUTH;
		case JoystickButtonPosition::FaceLeft: return SDL_GAMEPAD_BUTTON_WEST;
		case JoystickButtonPosition::Select: return SDL_GAMEPAD_BUTTON_BACK;
		case JoystickButtonPosition::Start: return SDL_GAMEPAD_BUTTON_START;
		case JoystickButtonPosition::BumperLeft: return SDL_GAMEPAD_BUTTON_LEFT_SHOULDER;
		case JoystickButtonPosition::BumperRight: return SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER;
		case JoystickButtonPosition::TriggerLeft: return SDL_GAMEPAD_BUTTON_COUNT;
		case JoystickButtonPosition::TriggerRight: return SDL_GAMEPAD_BUTTON_COUNT + 1;
		case JoystickButtonPosition::LeftStick: return SDL_GAMEPAD_BUTTON_LEFT_STICK;
		case JoystickButtonPosition::RightStick: return SDL_GAMEPAD_BUTTON_RIGHT_STICK;
		case JoystickButtonPosition::Accept: return SDL_GAMEPAD_BUTTON_SOUTH;
		case JoystickButtonPosition::Cancel: return SDL_GAMEPAD_BUTTON_EAST;
		case JoystickButtonPosition::DPadUp: return SDL_GAMEPAD_BUTTON_DPAD_UP;
		case JoystickButtonPosition::DPadRight: return SDL_GAMEPAD_BUTTON_DPAD_RIGHT;
		case JoystickButtonPosition::DPadDown: return SDL_GAMEPAD_BUTTON_DPAD_DOWN;
		case JoystickButtonPosition::DPadLeft: return SDL_GAMEPAD_BUTTON_DPAD_LEFT;
		case JoystickButtonPosition::System: return SDL_GAMEPAD_BUTTON_GUIDE;
		default: throw Exception("Invalid parameter", HalleyExceptions::InputPlugin);
	}
}

#ifdef WITH_GDK
Halley::String InputGameControllerSDL3::getButtonName(int code) const
{
	auto buttons = std::array<const char*, 17>{
		"xbox_a",
		"xbox_b",
		"xbox_x",
		"xbox_y",
		"xbox_lb",
		"xbox_rb",
		"xbox_lsb",
		"xbox_rsb",
		"xbox_back",
		"xbox_start",
		"xbox_lt",
		"xbox_rt",
		"xbox_dpad_up",
		"xbox_dpad_right",
		"xbox_dpad_down",
		"xbox_dpad_left",
		"xbox_guide"
	};
	if (code < buttons.size())
	{
		return buttons[code];
	}
	if (code == 26)
	{
		return "xbox_lt";
	}
	if (code == 27)
	{
		return "xbox_rt";
	}

	return InputJoystick::getButtonName(code);
}
#endif

void InputGameControllerSDL3::processEvent(const SDL_Event& event)
{
	if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN || event.type == SDL_EVENT_GAMEPAD_BUTTON_UP) {
		bool down = event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN;
		onButtonStatus(event.gbutton.button, down);
		// Map DPAD button events to the hat button status
		if (event.gbutton.button >= SDL_GAMEPAD_BUTTON_DPAD_UP && event.gbutton.button <= SDL_GAMEPAD_BUTTON_DPAD_RIGHT) {
			hats[0]->onButtonStatus(0, down && event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_UP);
			hats[0]->onButtonStatus(1, down && event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
			hats[0]->onButtonStatus(2, down && event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_DOWN);
			hats[0]->onButtonStatus(3, down && event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_LEFT);
		}
	} else if (event.type == SDL_EVENT_GAMEPAD_AXIS_MOTION) {
		const auto axis = event.gaxis.axis;
		const auto value = static_cast<float>(event.gaxis.value) / static_cast<float>(std::numeric_limits<int16_t>::max());
		axes[axis] = value;
		if (axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER) {
			onButtonStatus(SDL_GAMEPAD_BUTTON_COUNT, value > 0.5f);
		} else if (axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) {
			onButtonStatus(SDL_GAMEPAD_BUTTON_COUNT + 1, value > 0.5f);
		}
	}
}

void InputGameControllerSDL3::doSetVibration(float low, float high)
{
	if (controller) {
        SDL_RumbleGamepad(controller, static_cast<uint16_t>(low * 65535), static_cast<uint16_t>(high * 65535), 100);
	}
}