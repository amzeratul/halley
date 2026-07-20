#include "halley/input/input_device.h"

#include "halley/support/logger.h"

using namespace Halley;

InputDevice::InputDevice()
{
	static uint16_t id = 1;
	deviceId = id++;
}

InputDevice::~InputDevice() = default;

std::string_view InputDevice::getName() const
{
	return "InputDevice";
}

bool InputDevice::isEnabled() const
{
	return true;
}

size_t InputDevice::getNumberButtons() const
{
	return 0;
}

size_t InputDevice::getNumberAxes() const
{
	return 0;
}

size_t InputDevice::getNumberMotionSensors() const
{
	return 0;
}

String InputDevice::getButtonName(int code) const
{
	return "";
}

String InputDevice::getAxisName(int index) const
{
	return "";
}

int InputDevice::getButtonAtPosition(JoystickButtonPosition position) const
{
	return -1;
}

int InputDevice::getAxisAtPosition(JoystickAxisPosition position) const
{
	switch (position) {
	case JoystickAxisPosition::LeftStickX:
		return 0;
	case JoystickAxisPosition::LeftStickY:
		return 1;
	case JoystickAxisPosition::RightStickX:
		return 2;
	case JoystickAxisPosition::RightStickY:
		return 3;
	case JoystickAxisPosition::TriggerLeft:
		return 4;
	case JoystickAxisPosition::TriggerRight:
		return 5;
	}
	return -1;
}

std::optional<JoystickButtonPosition> InputDevice::getPositionForButton(int code) const
{
	return std::nullopt;
}

std::optional<JoystickAxisPosition> InputDevice::getPositionForAxis(int code) const
{
	if (code >= 0 && code <= 5) {
		return static_cast<JoystickAxisPosition>(code);
	}
	return std::nullopt;
}

bool InputDevice::isAnyButtonPressed() const
{
	return false;
}

bool InputDevice::isAnyButtonPressedRepeat() const
{
	return false;
}

bool InputDevice::isAnyButtonReleased() const
{
	return false;
}

bool InputDevice::isAnyButtonDown() const
{
	return false;
}

bool InputDevice::isButtonPressed(InputButton code) const
{
	return false;
}

bool InputDevice::isButtonPressedRepeat(InputButton code) const
{
	return false;
}

bool InputDevice::isButtonReleased(InputButton code) const
{
	return false;
}

bool InputDevice::isButtonDown(InputButton code) const
{
	return false;
}

KeyMods InputDevice::getKeyMods() const
{
	return KeyMods::None;
}

bool InputDevice::hasAnyInput() const
{
	if (isAnyButtonPressed() || isAnyButtonReleased()) {
		return true;
	}

	for (int i = 0; i < getNumberAxes(); ++i) {
		if (std::fabs(getAxis(i)) > 0.1f) {
			return true;
		}
	}

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

void InputDevice::clearAxes()
{
}

float InputDevice::getAxis(int) const
{
	return 0;
}

int InputDevice::getAxisRepeat(int) const
{
	return 0;
}

size_t InputDevice::getNumberHats() const
{
	return 0;
}

std::optional<int> InputDevice::getPlayerIndex() const
{
	return {};
}

bool InputDevice::hasLED() const
{
	return false;
}

void InputDevice::setLED(Colour4c colour) const
{
}

Vector<int> InputDevice::getButtonsPressed() const
{
	Vector<int> result;

	const auto n = static_cast<int>(getNumberButtons());
	for (int i = 0; i < n; ++i) {
		if (isButtonPressed(i)) {
			result += i;
		}
	}

	return result;
}

Vector<int> InputDevice::getButtonsReleased() const
{
	Vector<int> result;

	const auto n = static_cast<int>(getNumberButtons());
	for (int i = 0; i < n; ++i) {
		if (isButtonReleased(i)) {
			result += i;
		}
	}

	return result;
}

Vector<int> InputDevice::getButtonsDown() const
{
	Vector<int> result;

	const auto n = static_cast<int>(getNumberButtons());
	for (int i = 0; i < n; ++i) {
		if (isButtonDown(i)) {
			result += i;
		}
	}

	return result;
}

Vector<std::pair<int, JoystickAxisDirection>> InputDevice::getAxesMoved(float threshold) const
{
	Vector<std::pair<int, JoystickAxisDirection>> result;

	const auto n = static_cast<int>(getNumberAxes());
	for (int i = 0; i < n; ++i) {
		const float value = getAxis(i);
		if (std::abs(value) >= threshold) {
			result += { i, value > 0 ? JoystickAxisDirection::Positive : JoystickAxisDirection::Negative };
		}
	}

	return result;
}

std::shared_ptr<InputDevice> InputDevice::getHat(int) const
{
	return {};
}

std::pair<float, float> InputDevice::getVibration() const
{
	return {};
}

void InputDevice::setVibration(float low, float high)
{
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

Vector2f InputDevice::getWheelMove() const
{
	return {};
}

Vector2i InputDevice::getWheelMoveDiscrete() const
{
	return {};
}

const InputMotionSensor& InputDevice::getMotionSensor(int n) const
{
	static InputMotionSensor dummy;
	return dummy;
}

void InputDevice::setParent(const std::shared_ptr<InputDevice>& parent)
{
}

std::shared_ptr<InputDevice> InputDevice::getParent() const
{
	return {};
}

bool InputDevice::isManual() const
{
	return false;
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

		const auto interval = std::array<Time, 3>{std::numeric_limits<float>::max(), repeatInterval0, repeatInterval1}[intervalIdx];
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
