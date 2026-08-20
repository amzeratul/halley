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

#include "halley/input/input_virtual.h"
#include "halley/input/input_manual.h"
#include <set>
#include <algorithm>
#include <utility>

#include "halley/utils/algorithm.h"
#include "halley/input/input_exclusive.h"

using namespace Halley;

InputVirtual::InputVirtual(int nButtons, int nAxes, InputType type)
	: lastDeviceFrozen(false)
	, repeatDelayFirst(0.20f)
	, repeatDelayHold(0.10f)
	, type(type)
{
	buttons.resize(nButtons);
	axes.resize(nAxes);
}

InputVirtual::~InputVirtual()
{
	for (auto& button: exclusiveButtons) {
		button->parent = nullptr;
		button->activeBinds.clear();
	}
}

bool InputVirtual::isEnabled() const
{
	for (const auto& d: getAllDevices()) {
		if (d->isEnabled()) {
			return true;
		}
	}
	return false;
}

size_t InputVirtual::getNumberHats() const
{
	return 0;
}

std::shared_ptr<InputDevice> InputVirtual::getHat(int) const
{
	return std::shared_ptr<InputDevice>();
}

size_t InputVirtual::getNumberButtons() const
{
	return buttons.size();
}

size_t InputVirtual::getNumberAxes() const
{
	return axes.size();
}

size_t InputVirtual::getNumberMotionSensors() const
{
	return motionSensors.size();
}

bool InputVirtual::isAnyButtonPressed() const
{
	for (auto& binds : buttons) {
		for (auto& bind : binds) {
			if (bind.device->isAnyButtonPressed()) {
				return true;
			}
		}
	}
	return false;
}

bool InputVirtual::isAnyButtonReleased() const
{
	for (auto& binds : buttons) {
		for (auto& bind : binds) {
			if (bind.device->isAnyButtonReleased()) {
				return true;
			}
		}
	}
	return false;
}

bool InputVirtual::isAnyButtonDown() const
{
	for (auto& binds : buttons) {
		for (auto& bind : binds) {
			if (bind.device->isAnyButtonDown()) {
				return true;
			}
		}
	}
	return false;
}

bool InputVirtual::isButtonPressed(InputButton code) const
{
	if (code < 0 || code >= static_cast<int>(buttons.size())) {
		return false;
	}
	for (auto& bind : buttons.at(code)) {
		if (bind.isButtonPressed()) {
			return true;
		}
	}
	return false;
}

bool InputVirtual::isButtonPressedRepeat(InputButton code) const
{
	if (code < 0 || code >= static_cast<int>(buttons.size())) {
		return false;
	}
	for (auto& bind : buttons.at(code)) {
		if (bind.isButtonPressedRepeat()) {
			return true;
		}
	}
	return false;
}

bool InputVirtual::isButtonReleased(InputButton code) const
{
	if (code < 0 || code >= static_cast<int>(buttons.size())) {
		return false;
	}
	for (auto& bind : buttons.at(code)) {
		if (bind.isButtonReleased()) {
			return true;
		}
	}
	return false;
}

bool InputVirtual::isButtonDown(InputButton code) const
{
	if (code < 0 || code >= static_cast<int>(buttons.size())) {
		return false;
	}
	for (auto& bind : buttons.at(code)) {
		if (bind.isButtonDown()) {
			return true;
		}
	}
	return false;
}

bool InputVirtual::isButtonPressed(InputButton code, gsl::span<const uint32_t> activeBinds)
{
	if (code < 0 || code >= static_cast<int>(buttons.size())) {
		return false;
	}
	refreshExclusives();
	for (auto& bind : buttons.at(code)) {
		if (checkBinds(activeBinds, bind) && bind.isButtonPressed()) {
			return true;
		}
	}
	return false;
}

bool InputVirtual::isButtonPressedRepeat(InputButton code, gsl::span<const uint32_t> activeBinds)
{
	if (code < 0 || code >= static_cast<int>(buttons.size())) {
		return false;
	}
	refreshExclusives();
	for (auto& bind : buttons.at(code)) {
		if (checkBinds(activeBinds, bind) && bind.isButtonPressedRepeat()) {
			return true;
		}
	}
	return false;
}

bool InputVirtual::isButtonReleased(InputButton code, gsl::span<const uint32_t> activeBinds)
{
	if (code < 0 || code >= static_cast<int>(buttons.size())) {
		return false;
	}
	refreshExclusives();
	for (auto& bind : buttons.at(code)) {
		if (checkBinds(activeBinds, bind) && bind.isButtonReleased()) {
			return true;
		}
	}
	return false;
}

bool InputVirtual::isButtonDown(InputButton code, gsl::span<const uint32_t> activeBinds)
{
	if (code < 0 || code >= static_cast<int>(buttons.size())) {
		return false;
	}
	refreshExclusives();
	for (auto& bind : buttons.at(code)) {
		if (checkBinds(activeBinds, bind) && bind.isButtonDown()) {
			return true;
		}
	}
	return false;
}

float InputVirtual::getAxis(int n, gsl::span<const uint32_t> activeBinds)
{
	if (n < 0 || n >= static_cast<int>(axes.size())) {
		return false;
	}
	refreshExclusives();
	float value = 0;
	for (auto& bind: axes.at(n).binds) {
		if (checkBinds(activeBinds, bind)) {
			value += bind.getAxis();
		}
	}
	return value;
}

void InputVirtual::clearButton(InputButton code)
{
	auto& binds = buttons.at(code);
	for (auto& bind : binds) {
		bind.device->clearButton(bind.a);
		if (bind.b != -1) {
			bind.device->clearButton(bind.b);
		}
	}
}

void InputVirtual::clearButtonPress(InputButton code)
{
	auto& binds = buttons.at(code);
	for (auto& bind : binds) {
		bind.device->clearButtonPress(bind.a);
		if (bind.b != -1) {
			bind.device->clearButtonPress(bind.b);
		}
	}
}

void InputVirtual::clearButtonRelease(InputButton code)
{
	auto& binds = buttons.at(code);
	for (auto& bind : binds) {
		bind.device->clearButtonRelease(bind.a);
		if (bind.b != -1) {
			bind.device->clearButtonRelease(bind.b);
		}
	}
}

float InputVirtual::getAxis(int n) const
{
	if (n < 0 || n >= static_cast<int>(axes.size())) {
		return false;
	}
	return axes.at(n).getValue();
}

int InputVirtual::getAxisRepeat(int n) const
{
	if (n < 0 || n >= static_cast<int>(axes.size())) {
		return false;
	}
	return axes.at(n).curRepeatValue;
}

void InputVirtual::bindButton(ConvertibleTo<int> n, spInputDevice device, ConvertibleTo<int> deviceN, bool toggle)
{
	if (!lastDeviceSet || (device->getInputType() == InputType::Gamepad && lastJoystickType == JoystickType::None)) {
		setLastDevice(device);
	}
	buttons.at(n.value).push_back(Bind(std::move(device), deviceN.value, -1, toggle ? Bind::Mode::ButtonToggle : Bind::Mode::Button));
	exclusiveDirty = true;
}

void InputVirtual::bindButton(ConvertibleTo<int> n, spInputDevice device, KeyCode deviceButton, std::optional<KeyMods> mods, bool toggle)
{
	if (!lastDeviceSet || (device->getInputType() == InputType::Gamepad && lastJoystickType == JoystickType::None)) {
		setLastDevice(device);
	}
	buttons.at(n.value).push_back(Bind(std::move(device), static_cast<int>(deviceButton), -1, toggle ? Bind::Mode::ButtonToggle : Bind::Mode::Button, mods));
	exclusiveDirty = true;
}

void InputVirtual::bindButtonChord(ConvertibleTo<int> n, spInputDevice device, ConvertibleTo<int> deviceButton0, ConvertibleTo<int> deviceButton1, bool toggle)
{
	if (!lastDeviceSet || (device->getInputType() == InputType::Gamepad && lastJoystickType == JoystickType::None)) {
		setLastDevice(device);
	}
	buttons.at(n.value).push_back(Bind(std::move(device), deviceButton0.value, deviceButton1.value, toggle ? Bind::Mode::ButtonToggle : Bind::Mode::Button));
	exclusiveDirty = true;
}

void InputVirtual::bindAxis(ConvertibleTo<int> n, spInputDevice device, ConvertibleTo<int> deviceN, float scale)
{
	if (!lastDeviceSet || (device->getInputType() == InputType::Gamepad && lastJoystickType == JoystickType::None)) {
		setLastDevice(device);
	}
	axes.at(n.value).binds.push_back(Bind(std::move(device), deviceN.value, -1, Bind::Mode::Axis, {}, scale));
	exclusiveDirty = true;
}

void InputVirtual::bindAxisButton(ConvertibleTo<int> n, spInputDevice device, ConvertibleTo<int> negativeButton, ConvertibleTo<int> positiveButton)
{
	if (!lastDeviceSet || (device->getInputType() == InputType::Gamepad && lastJoystickType == JoystickType::None)) {
		setLastDevice(device);
	}
	axes.at(n.value).binds.push_back(Bind(std::move(device), negativeButton.value, positiveButton.value, Bind::Mode::AxisEmulation));
	exclusiveDirty = true;
}

void InputVirtual::bindAxisButton(ConvertibleTo<int> n, spInputDevice device, KeyCode negativeButton, KeyCode positiveButton, std::optional<KeyMods> mods)
{
	if (!lastDeviceSet || (device->getInputType() == InputType::Gamepad && lastJoystickType == JoystickType::None)) {
		setLastDevice(device);
	}
	axes.at(n.value).binds.push_back(Bind(std::move(device), static_cast<int>(negativeButton), static_cast<int>(positiveButton), Bind::Mode::AxisEmulation, mods));
	exclusiveDirty = true;
}

void InputVirtual::bindVibrationOverride(spInputDevice joy)
{
	vibrationOverride = std::move(joy);
}

void InputVirtual::unbindButton(int n)
{
	buttons.at(n).clear();
	exclusiveDirty = true;
}

void InputVirtual::unbindAxis(int n)
{
	axes.at(n).binds.clear();
	exclusiveDirty = true;
}

void InputVirtual::clearBindings()
{
	for (auto& button : buttons) {
		button.clear();
	}
	for (auto& axe : axes) {
		axe.binds.clear();
	}
	motionSensors.clear();
	vibrationOverride = spInputDevice();
}

std::pair<float, float> InputVirtual::getVibration() const
{
	const auto& dev = vibrationOverride ? vibrationOverride.get() : getLastDevice();
	if (dev) {
		return dev->getVibration();
	} else {
		return {};
	}
}

void InputVirtual::setVibration(float low, float high)
{
	const auto& dev = vibrationOverride ? vibrationOverride.get() : getLastDevice();
	if (dev) {
		dev->setVibration(low, high);
	}
}

void InputVirtual::vibrate(spInputVibration vib)
{
	const auto& dev = vibrationOverride ? vibrationOverride.get() : getLastDevice();
	if (dev) {
		dev->vibrate(vib);
	}
}

void InputVirtual::stopVibrating()
{
	const auto& dev = vibrationOverride ? vibrationOverride.get() : getLastDevice();
	if (dev) {
		dev->stopVibrating();
	}
}

Vector2f InputVirtual::getPosition() const
{
	return position;
}

void InputVirtual::setPosition(Vector2f pos)
{
	position = pos;
}

void InputVirtual::setPositionLimits(Rect4f limits)
{
	positionLimits = limits;
}

void InputVirtual::setPositionLimits()
{
	positionLimits.reset();
}

Vector2f InputVirtual::getWheelMove() const
{
	Vector2f val;
	for (const auto& w: wheels) {
		val += w->getWheelMove();
	}
	return val;
}

Vector2i InputVirtual::getWheelMoveDiscrete() const
{
	Vector2i val;
	for (const auto& w: wheels) {
		val += w->getWheelMoveDiscrete();
	}
	return val;
}

const InputMotionSensor& InputVirtual::getMotionSensor(int n) const
{
	if (n >= 0 && n < motionSensors.size()) {
		return motionSensors[n].device->getMotionSensor(motionSensors[n].sensorId);
	}
	return InputDevice::getMotionSensor(n);
}

void InputVirtual::bindHat(int leftRight, int upDown, spInputDevice hat)
{
	bindAxisButton(leftRight, hat, 3, 1);
	bindAxisButton(upDown, hat, 0, 2);
}

void InputVirtual::bindPosition(spInputDevice device)
{
	positions.push_back(PositionBindData(std::move(device)));
}

void InputVirtual::bindPositionRelative(spInputDevice device, int axisX, int axisY, float speed)
{
	positions.push_back(PositionBindData(std::move(device), axisX, axisY, speed));
}

void InputVirtual::bindWheel(spInputDevice device)
{
	if (!std_ex::contains(wheels, device)) {
		wheels.push_back(std::move(device));
	}
}

void InputVirtual::bindMotionSensor(ConvertibleTo<int> n, spInputDevice device, ConvertibleTo<int> deviceMotionControls)
{
	motionSensors.resize(std::max<size_t>(motionSensors.size(), n.value + 1));
	auto& s = motionSensors[n.value];
	s.device = std::move(device);
	s.sensorId = deviceMotionControls.value;
}

String InputVirtual::getButtonName(int code) const
{
	const auto& binds = buttons.at(code);
	if (!binds.empty()) {
		auto* last = getLastDevice();
		for (const auto& bind: binds) {
			if (bind.device.get() == last) {
				return bind.device->getButtonName(bind.a);
			}
		}
		return binds.front().device->getButtonName(binds.front().a);
	} else if (type == InputType::Mouse) {
		return toString(static_cast<MouseButton>(code));
	} else {
		return "";
	}
}

void InputVirtual::update(Time t)
{
	updateLastDevice();

	for (auto& axis: axes) {
		axis.curRepeatValue = axis.repeat.update(axis.getValue(), t);
	}
	for (auto& button: buttons) {
		for (auto& bind: button) {
			bind.updateToggle();
		}
	}

	for (auto& pos: positions) {
		if (pos.direct) {
			auto posNow = pos.device->getPosition();
			if ((pos.lastRead - posNow).squaredLength() > 0.001f) {
				pos.lastRead = posNow;
				position = posNow;
			}
		} else {
			Vector2f delta(pos.device->getAxis(pos.axisX), pos.device->getAxis(pos.axisY));
			position += delta * float(pos.speed * t);
		}
	}
	if (positionLimits) {
		position = positionLimits->getClosestPoint(position);
	}

	for (auto& button: exclusiveButtons) {
		button->update(t);
	}
	for (auto& axis: exclusiveAxes) {
		axis->update(t);
	}
}

InputDevice* InputVirtual::getLastDevice() const
{
	if (auto last = lastDevice.lock()) {
		return last.get();
	} else {
		// No last device, get the first thing we can find
		for (auto& binds: buttons) {
			for (auto& bind: binds) {
				if (bind.device) {
					return bind.device.get();
				}
			}
		}
		return nullptr;
	}
}

void InputVirtual::setLastDeviceToType(InputType type)
{
	for (auto& binds: buttons) {
		for (auto& bind: binds) {
			if (bind.device->getInputType() == type) {
				lastDevice = bind.device;
				return;
			}
		}
	}
}

void InputVirtual::updateLastDevice()
{
	if (!lastDeviceFrozen) {
		for (auto& buttonBinds: buttons) {
			for (auto& bind: buttonBinds) {
				if (bind.device && !bind.device->isManual()) {
					if (bind.mode != Bind::Mode::AxisEmulation && bind.device->isButtonPressed(bind.a)) {
						setLastDevice(bind.device);
						return;
					}
				}
			}
		}
		for (auto& axisBind: axes) {
			for (auto& bind: axisBind.binds) {
				if (bind.device && !bind.device->isManual()) {
					if ((bind.mode != Bind::Mode::AxisEmulation && fabs(bind.device->getAxis(bind.a)) > 0.1f)
						|| (bind.mode == Bind::Mode::AxisEmulation && bind.device->isButtonDown(bind.a))
						|| (bind.mode == Bind::Mode::AxisEmulation && bind.device->isButtonDown(bind.b))) {
						setLastDevice(bind.device);
						return;
					}
				}
			}
		}
	}
}

InputVirtual::Bind::Bind(spInputDevice d, int a, int b, Mode mode, std::optional<KeyMods> mods, float scale)
	: device(std::move(d))
	, a(a)
	, b(b)
	, mode(mode)
	, mods(mods)
	, scale(scale)
{}

bool InputVirtual::Bind::isButtonPressed() const
{
	if (!checkMods()) {
		return false;
	}
	if (mode == Mode::Button) {
		if (b == -1) {
			// Single bind
			if (device->isButtonPressed(a)) {
				return true;
			}
		} else {
			// Chord bind
			if ((device->isButtonPressed(a) && device->isButtonDown(b)) || (device->isButtonPressed(b) && device->isButtonDown(a))) {
				return true;
			}
		}
	}
	return false;
}

bool InputVirtual::Bind::isButtonPressedRepeat() const
{
	if (!checkMods()) {
		return false;
	}
	if (mode == Mode::Button) {
		if (b == -1) {
			// Single bind
			if (device->isButtonPressedRepeat(a)) {
				return true;
			}
		} else {
			// Chord bind
			if (device->isButtonPressedRepeat(a) && device->isButtonDown(b)) {
				return true;
			}
		}
	}
	return false;
}

bool InputVirtual::Bind::isButtonReleased() const
{
	if (!checkMods()) {
		return false;
	}

	if (mode == Mode::Button) {
		if (b == -1) {
			// Single bind
			if (device->isButtonReleased(a)) {
				return true;
			}
		} else {
			// Chord bind
			const bool aReleased = device->isButtonReleased(a);
			const bool bReleased = device->isButtonReleased(b);
			if ((aReleased && bReleased) || (aReleased && device->isButtonDown(b)) || (bReleased && device->isButtonDown(a))) {
				return true;
			}
		}
	}
	return false;
}

bool InputVirtual::Bind::isButtonDown() const
{
	if (!checkMods()) {
		return false;
	}

	if (mode == Mode::ButtonToggle) {
		return toggleState;
	} else if (mode == Mode::Button) {
		if (b == -1) {
			if (device->isButtonDown(a)) {
				return true;
			}
		} else {
			if (device->isButtonDown(a) && device->isButtonDown(b)) {
				return true;
			}
		}
	}
	return false;
}

bool InputVirtual::Bind::checkMods() const
{
	return !mods || device->getKeyMods() == *mods;
}

float InputVirtual::Bind::getAxis() const
{
	if (mode == Mode::AxisEmulation) {
		const int left = device->isButtonDown(a) ? 1 : 0;
		const int right = device->isButtonDown(b) ? 1 : 0;
		return static_cast<float>(right - left) * scale;
	} else {
		return device->getAxis(a) * scale;
	}
}

void InputVirtual::Bind::updateToggle()
{
	if (mode == Mode::ButtonToggle) {
		if (device->isButtonPressed(a)) {
			toggleState = !toggleState;
		}
	}
}

std::pair<uint32_t, uint32_t> InputVirtual::Bind::getPhysicalButtonIds() const
{
	if (mode == Mode::Axis) {
		const auto idA = (static_cast<uint32_t>(device->getId()) << 16) | static_cast<uint32_t>(0x100) | static_cast<uint32_t>(a);
		return { idA, 0 };
	} else {
		const auto idA = (static_cast<uint32_t>(device->getId()) << 16) | static_cast<uint32_t>(a);
		if (b != -1) {
			const auto idB = (static_cast<uint32_t>(device->getId()) << 16) | static_cast<uint32_t>(b);
			HalleyAssertDev(idB != 0); // If this is ever zero, it will conflict with "empty" id below
			return { idA, idB };
		} else {
			return { idA, 0 };
		}
	}
}


InputVirtual::AxisData::AxisData() = default;

InputVirtual::AxisData::AxisData(Vector<Bind> b)
	: binds(std::move(b))
{}

float InputVirtual::AxisData::getValue() const
{
	float value = 0;
	for (const auto& bind: binds) {
		value += bind.getAxis();
	}
	return value;
}

InputVirtual::PositionBindData::PositionBindData() = default;

InputVirtual::PositionBindData::PositionBindData(spInputDevice device)
	: device(std::move(device))
	, direct(true)
{}

InputVirtual::PositionBindData::PositionBindData(spInputDevice device, int axisX, int axisY, float speed)
	: device(std::move(device))
	, axisX(axisX)
	, axisY(axisY)
	, speed(speed)
{}

std::set<spInputDevice> InputVirtual::getAllDevices() const
{
	std::set<spInputDevice> devices;

	for (const auto& axisBind: axes) {
		for (const auto& bind: axisBind.binds) {
			if (bind.device) {
				devices.insert(bind.device);
			}
		}
	}
	for (const auto& buttonBinds: buttons) {
		for (const auto& bind: buttonBinds) {
			if (bind.device) {
				devices.insert(bind.device);
			}
		}
	}

	return devices;
}

void InputVirtual::setLastDeviceFreeze(bool frozen)
{
	lastDeviceFrozen = frozen;
}

void InputVirtual::setRepeat(float first, float hold)
{
	repeatDelayFirst = first;
	repeatDelayHold = hold;
}

void InputVirtual::setJoystickTypeOverride(std::optional<JoystickType> type)
{
	joystickTypeOverride = type;
}

JoystickType InputVirtual::getJoystickType() const
{
	return joystickTypeOverride.value_or(lastJoystickType);
}

InputType InputVirtual::getInputType() const
{
	return type;
}

void InputVirtual::setLastDevice(const std::shared_ptr<InputDevice>& device)
{
	lastDeviceSet = true;
	if (const auto parent = device->getParent()) {
		setLastDevice(parent);
	} else {
		lastDevice = device;
		if (device->getInputType() == InputType::Gamepad) {
			lastJoystickType = device->getJoystickType();
		}
	}
}

std::unique_ptr<InputExclusiveButton> InputVirtual::makeExclusiveButton(InputButton button, InputPriority priority, const InputLabel& label)
{
	auto exclusive = std::unique_ptr<InputExclusiveButton>(new InputExclusiveButton(*this, priority, button, label));
	addExclusiveButton(*exclusive);
	return exclusive;
}

std::unique_ptr<InputExclusiveAxis> InputVirtual::makeExclusiveAxis(int axis, InputPriority priority, const InputLabel& label)
{
	auto exclusive = std::unique_ptr<InputExclusiveAxis>(new InputExclusiveAxis(*this, priority, axis, label));
	addExclusiveAxis(*exclusive);
	return exclusive;
}

Vector<InputVirtual::ExclusiveButtonInfo> InputVirtual::getExclusiveButtonLabels(InputDevice* preferredDevice)
{
	refreshExclusives();

	if (!preferredDevice) {
		preferredDevice = getLastDevice();
	}

	Vector<ExclusiveButtonInfo> result;
	for (const auto& button: exclusiveButtons) {
		if (!button->activeBinds.empty()) {
			const auto& label = button->getLabel();
			if (!label.label.isEmpty()) {
				auto [physicalDevice, physicalButton] = getPhysicalButton(*button, preferredDevice);
				if (physicalDevice) {
					auto info = ExclusiveButtonInfo{ button->button, label, physicalDevice, physicalButton };
					result.emplace_back(std::move(info));
				}
			}
		}
	}
	return result;
}

std::pair<InputDevice*, int> InputVirtual::getPhysicalButton(ConvertibleTo<int> button, InputDevice* device) const
{
	if (!device) {
		device = getLastDevice();
	}

	auto isCompatible = [](InputDevice& a, InputDevice& b) -> bool
	{
		const auto typeA = a.getInputType();
		const auto typeB = b.getInputType();
		return (typeA == typeB)
			|| (typeA == InputType::Keyboard && typeB == InputType::Mouse)
			|| (typeA == InputType::Mouse && typeB == InputType::Keyboard);
	};

	std::pair<InputDevice*, int> bestResult = { nullptr, 0 };
	int bestScore = 0;

	for (const auto& binding : buttons.at(button.value)) {
		if (binding.device.get() == device) {
			return { binding.device.get(), binding.a };
		}

		const int score = isCompatible(*binding.device, *device) ? 1 : 0;
		if (score > bestScore) {
			bestResult = { binding.device.get(), binding.a };
			bestScore = score;
		}
	}

	return bestResult;
}

std::pair<InputDevice*, int> InputVirtual::getPhysicalAxis(ConvertibleTo<int> axis, InputDevice* device) const
{
	if (!device) {
		device = getLastDevice();
	}

	auto isCompatible = [](InputDevice& a, InputDevice& b) -> bool
	{
		const auto typeA = a.getInputType();
		const auto typeB = b.getInputType();
		return (typeA == typeB)
			|| (typeA == InputType::Keyboard && typeB == InputType::Mouse)
			|| (typeA == InputType::Mouse && typeB == InputType::Keyboard);
	};

	std::pair<InputDevice*, int> bestResult = { nullptr, 0 };
	int bestScore = 0;

	for (const auto& binding : axes.at(axis.value).binds) {
		if (binding.device.get() == device) {
			return { binding.device.get(), binding.a };
		}

		const int score = isCompatible(*binding.device, *device) ? 1 : 0;
		if (score > bestScore) {
			bestResult = { binding.device.get(), binding.a };
			bestScore = score;
		}
	}

	return bestResult;
}

std::pair<InputDevice*, int> InputVirtual::getPhysicalButton(const InputExclusiveButton& button, InputDevice* device) const
{
	if (!device) {
		device = getLastDevice();
	}

	auto isCompatible = [](InputDevice& a, InputDevice& b) -> bool
	{
		const auto typeA = a.getInputType();
		const auto typeB = b.getInputType();
		return (typeA == typeB)
			|| (typeA == InputType::Keyboard && typeB == InputType::Mouse)
			|| (typeA == InputType::Mouse && typeB == InputType::Keyboard);
	};

	std::pair<InputDevice*, int> bestResult = { nullptr, 0 };
	int bestScore = 0;

	for (const auto& binding: buttons[button.button]) {
		if (checkBinds(button.activeBinds, binding)) {
			if (binding.device.get() == device) {
				return { binding.device.get(), binding.a };
			}

			const int score = isCompatible(*binding.device, *device) ? 1 : 0;
			if (score > bestScore) {
				bestResult = { binding.device.get(), binding.a };
				bestScore = score;
			}
		}
	}

	return bestResult;
}

std::pair<InputDevice*, int> InputVirtual::getPhysicalAxis(const InputExclusiveAxis& axis, InputDevice* device) const
{
	if (!device) {
		device = getLastDevice();
	}

	auto isCompatible = [](InputDevice& a, InputDevice& b) -> bool
	{
		const auto typeA = a.getInputType();
		const auto typeB = b.getInputType();
		return (typeA == typeB)
			|| (typeA == InputType::Keyboard && typeB == InputType::Mouse)
			|| (typeA == InputType::Mouse && typeB == InputType::Keyboard);
	};

	std::pair<InputDevice*, int> bestResult = { nullptr, 0 };
	int bestScore = 0;

	for (const auto& binding: axes[axis.axis].binds) {
		if (checkBinds(axis.activeBinds, binding)) {
			if (binding.device.get() == device) {
				return { binding.device.get(), binding.a };
			}

			const int score = isCompatible(*binding.device, *device) ? 1 : 0;
			if (score > bestScore) {
				bestResult = { binding.device.get(), binding.a };
				bestScore = score;
			}
		}
	}

	return bestResult;
}

bool InputVirtual::checkBinds(gsl::span<const uint32_t> activeBinds, const Bind& bind) const
{
	const auto ids = bind.getPhysicalButtonIds();
	return std_ex::contains(activeBinds, ids.first) && (!ids.second || std_ex::contains(activeBinds, ids.second));
}

void InputVirtual::clearPresses()
{
	for (auto& axis: axes) {
		axis.curRepeatValue = 0;
	}
}

void InputVirtual::addExclusiveButton(InputExclusiveButton& exclusive)
{
	exclusiveButtons.push_back(&exclusive);
	exclusiveDirty = true;
}

void InputVirtual::removeExclusiveButton(InputExclusiveButton& exclusive)
{
	std_ex::erase(exclusiveButtons, &exclusive);
	exclusiveDirty = true;
}

void InputVirtual::addExclusiveAxis(InputExclusiveAxis& exclusive)
{
	exclusiveAxes.push_back(&exclusive);
	exclusiveDirty = true;
}

void InputVirtual::removeExclusiveAxis(InputExclusiveAxis& exclusive)
{
	std_ex::erase(exclusiveAxes, &exclusive);
	exclusiveDirty = true;
}

void InputVirtual::refreshExclusives()
{
	if (exclusiveDirty) {
		exclusiveDirty = false;

		TempMemoryPool pool(32 * 1024);
		auto bindings = HashMapTemp<uint32_t, Vector<InputExclusiveBinding*>>(32, pool);
		
		for (const auto& exclusive: exclusiveButtons) {
			exclusive->activeBinds.clear();
			for (const auto& bind: buttons[exclusive->button]) {
				const auto ids = bind.getPhysicalButtonIds();
				bindings[ids.first].push_back(exclusive);
				if (ids.second) {
					bindings[ids.second].push_back(exclusive);
				}
			}
		}
		for (const auto& exclusive: exclusiveAxes) {
			exclusive->activeBinds.clear();
			for (const auto& bind: axes[exclusive->axis].binds) {
				const auto ids = bind.getPhysicalButtonIds();
				bindings[ids.first].push_back(exclusive);
				if (ids.second) {
					bindings[ids.second].push_back(exclusive);
				}
			}
		}

		for (auto& [bindId, exclusives]: bindings) {
			std::sort(exclusives.begin(), exclusives.end(), [] (const InputExclusiveBinding* a, const InputExclusiveBinding* b)
			{
				return a->getPriority() > b->getPriority();
			});
			exclusives[0]->getActiveBinds().push_back(bindId);
		}
	}
}
