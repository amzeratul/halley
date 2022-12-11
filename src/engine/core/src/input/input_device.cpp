#include "halley/core/input/input_device.h"

#include "halley/support/logger.h"

using namespace Halley;

InputDevice::InputDevice()
{
	static uint16_t id = 1;
	deviceId = id++;
}

InputDevice::~InputDevice() = default;

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

String InputDevice::getButtonName(int code) const
{
	return "";
}

bool InputDevice::isAnyButtonPressed()
{
	return false;
}

bool InputDevice::isAnyButtonPressedRepeat()
{
	return false;
}

bool InputDevice::isAnyButtonReleased()
{
	return false;
}

bool InputDevice::isAnyButtonDown()
{
	return false;
}

bool InputDevice::isButtonPressed(InputButton code)
{
	return false;
}

bool InputDevice::isButtonPressedRepeat(InputButton code)
{
	return false;
}

bool InputDevice::isButtonReleased(InputButton code)
{
	return false;
}

bool InputDevice::isButtonDown(InputButton code)
{
	return false;
}

void InputDevice::clearButton(InputButton code)
{
}

void InputDevice::clearButtonPress(InputButton code)
{
}

void InputDevice::clearButtonRelease(InputButton code)
{
}

void InputDevice::clearPresses()
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

InputType InputDevice::getInputType() const
{
	return InputType::None;
}

Vector2f InputDevice::getPosition() const
{
	return {};
}

void InputDevice::setPosition(Vector2f position)
{
}

int InputDevice::getWheelMove() const
{
	return 0;
}

void InputDevice::setParent(InputDevice*)
{
}

InputDevice* InputDevice::getParent() const
{
	return nullptr;
}

InputAxisRepeater::InputAxisRepeater(Time firstDelay, Time repeatInterval0, Time secondDelay, Time repeatInterval1)
	: firstDelay(firstDelay)
	, secondDelay(secondDelay)
	, repeatInterval0(repeatInterval0)
	, repeatInterval1(repeatInterval1)
{
}

int InputAxisRepeater::update(float value, Time t)
{
	const int intValue = value > 0.5f ? 1 : (value < -0.5f ? -1 : 0);
	const bool changed = intValue != lastValue;
	lastValue = intValue;

	if (intValue != 0) {
		const auto prevIntervalIdx = timeHeld > secondDelay ? 2 : (timeHeld > firstDelay ? 1 : 0);
		timeHeld += t;
		timeSinceLastRepeat += t;
		const auto intervalIdx = timeHeld > secondDelay ? 2 : (timeHeld > firstDelay ? 1 : 0);

		if (intervalIdx != prevIntervalIdx) {
			timeSinceLastRepeat = 0;
			return intValue;
		}

		const auto interval = std::array<Time, 3>{std::numeric_limits<Time>::infinity(), repeatInterval0, repeatInterval1}[intervalIdx];
		if (timeSinceLastRepeat > interval) {
			timeSinceLastRepeat -= interval;
			return intValue;
		} else {
			return changed ? intValue : 0;
		}
	} else {
		timeHeld = 0;
		timeSinceLastRepeat = 0;
		return 0;
	}
}
