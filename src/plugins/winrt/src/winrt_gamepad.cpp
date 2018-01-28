#include "winrt_gamepad.h"
using namespace Halley;

#include "winrt/Windows.Gaming.Input.h"
using namespace winrt::Windows::Gaming::Input;

WinRTGamepad::WinRTGamepad(int index)
	: index(index)
{
	// Axes
	axes.resize(6);
	axisAdjust = &defaultAxisAdjust;

	// Hat
	hats.resize(1);
	hats[0] = std::make_shared<InputButtonBase>(4);
	hats[0]->setParent(this);

	// Buttons
	init(12);
}

WinRTGamepad::~WinRTGamepad()
{
}

std::string WinRTGamepad::getName() const
{
	return "Gamepad #" + toString(index+1);
}

void WinRTGamepad::update(Time t)
{
	const bool active = index < int(Gamepad::Gamepads().Size());

	if (isEnabled() && !active) {
		clearAxes();
		clearPresses();
		hats[0]->clearPresses();
	}

	setEnabled(active);

	if (active) {
		Gamepad gamepad = Gamepad::Gamepads().GetAt(index);
		const GamepadReading reading = gamepad.GetCurrentReading();
		
		axes[0] = float(reading.LeftThumbstickX);
		axes[1] = -float(reading.LeftThumbstickY);
		axes[2] = float(reading.RightThumbstickX);
		axes[3] = -float(reading.RightThumbstickY);
		axes[4] = float(reading.LeftTrigger);
		axes[5] = float(reading.RightTrigger);

		auto b = reading.Buttons;
		onButtonStatus(0, (b & GamepadButtons::A) != GamepadButtons::None);
		onButtonStatus(1, (b & GamepadButtons::B) != GamepadButtons::None);
		onButtonStatus(2, (b & GamepadButtons::X) != GamepadButtons::None);
		onButtonStatus(3, (b & GamepadButtons::Y) != GamepadButtons::None);
		onButtonStatus(4, (b & GamepadButtons::LeftShoulder) != GamepadButtons::None);
		onButtonStatus(5, (b & GamepadButtons::RightShoulder) != GamepadButtons::None);
		onButtonStatus(6, (b & GamepadButtons::LeftThumbstick) != GamepadButtons::None);
		onButtonStatus(7, (b & GamepadButtons::RightThumbstick) != GamepadButtons::None);
		onButtonStatus(8, (b & GamepadButtons::View) != GamepadButtons::None);
		onButtonStatus(9, (b & GamepadButtons::Menu) != GamepadButtons::None);
		onButtonStatus(10, axes[4] > 0.5f);
		onButtonStatus(11, axes[5] > 0.5f);

		hats[0]->onButtonStatus(0, (b & GamepadButtons::DPadUp) != GamepadButtons::None);
		hats[0]->onButtonStatus(1, (b & GamepadButtons::DPadRight) != GamepadButtons::None);
		hats[0]->onButtonStatus(2, (b & GamepadButtons::DPadDown) != GamepadButtons::None);
		hats[0]->onButtonStatus(3, (b & GamepadButtons::DPadLeft) != GamepadButtons::None);
	}

	InputJoystick::update(t);
}

int WinRTGamepad::getButtonAtPosition(JoystickButtonPosition position) const
{
	switch (position) {
		case JoystickButtonPosition::FaceTop: return 3;
		case JoystickButtonPosition::FaceRight: return 1;
		case JoystickButtonPosition::FaceBottom: return 0;
		case JoystickButtonPosition::FaceLeft: return 2;
		case JoystickButtonPosition::Select: return 8;
		case JoystickButtonPosition::Start: return 9;
		case JoystickButtonPosition::BumperLeft: return 4;
		case JoystickButtonPosition::BumperRight: return 5;
		case JoystickButtonPosition::TriggerLeft: return 10;
		case JoystickButtonPosition::TriggerRight: return 11;
		case JoystickButtonPosition::LeftStick: return 6;
		case JoystickButtonPosition::RightStick: return 7;
		case JoystickButtonPosition::PlatformAcceptButton: return 0;
		case JoystickButtonPosition::PlatformCancelButton: return 1;
		default: throw Exception("Invalid parameter");
	}
}

void WinRTGamepad::setVibration(float low, float high)
{
	// TODO
}
