#include "halley/core/input/input_device.h"

using namespace Halley;

InputDevice::~InputDevice()
{
	
}

bool InputDevice::isEnabled() const
{
	return true;
}

size_t InputDevice::getNumberButtons()
{
	return 0;
}

size_t InputDevice::getNumberAxes()
{
	return 0;
}

String InputDevice::getButtonName(int code)
{
	return "";
}

bool InputDevice::isAnyButtonPressed()
{
	return false;
}

bool InputDevice::isAnyButtonReleased()
{
	return false;
}

bool InputDevice::isButtonPressed(int code)
{
	return false;
}

bool InputDevice::isButtonPressedRepeat(int code)
{
	return false;
}

bool InputDevice::isButtonReleased(int code)
{
	return false;
}

bool InputDevice::isButtonDown(int code)
{
	return false;
}

void InputDevice::clearButton(int code)
{
	
}

float InputDevice::getAxis(int)
{
	return 0;
}

int InputDevice::getAxisRepeat(int)
{
	return 0;
}

size_t InputDevice::getNumberHats()
{
	return 0;
}

std::shared_ptr<InputDevice> InputDevice::getHat(int)
{
	return {};
}

void InputDevice::vibrate(spInputVibration)
{
	
}

void InputDevice::stopVibrating()
{
	
}

JoystickType InputDevice::getJoystickType() const
{
	return JoystickType::None;
}

Vector2f InputDevice::getPosition() const
{
	return {};
}

int InputDevice::getWheelMove() const
{
	return 0;
}

int InputDevice::getNextLetter()
{
	return 0;
}

bool InputDevice::isSoftwareKeyboard()
{
	return false;
}

String InputDevice::getSoftwareKeyboardInput(const SoftwareKeyboardData& data)
{
	return data.initial;
}

void InputDevice::setParent(InputDevice*)
{
}

InputDevice* InputDevice::getParent() const
{
	return nullptr;
}
