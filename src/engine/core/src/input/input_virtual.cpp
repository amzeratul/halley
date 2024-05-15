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

size_t InputVirtual::getNumberHats()
{
	return 0;
}

std::shared_ptr<InputDevice> InputVirtual::getHat(int)
{
	return std::shared_ptr<InputDevice>();
}

size_t InputVirtual::getNumberButtons()
{
	return buttons.size();
}

size_t InputVirtual::getNumberAxes()
{
	return axes.size();
}

bool InputVirtual::isAnyButtonPressed()
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

bool InputVirtual::isAnyButtonReleased()
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

bool InputVirtual::isAnyButtonDown()
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

bool InputVirtual::isButtonPressed(InputButton code)
{
	for (auto& bind : buttons.at(code)) {
		if (bind.isButtonPressed()) {
			return true;
		}
	}
	return false;
}

bool InputVirtual::isButtonPressedRepeat(InputButton code)
{
	for (auto& bind : buttons.at(code)) {
		if (bind.isButtonPressedRepeat()) {
			return true;
		}
	}
	return false;
}

bool InputVirtual::isButtonReleased(InputButton code)
{
	for (auto& bind : buttons.at(code)) {
		if (bind.isButtonReleased()) {
			return true;
		}
	}
	return false;
}

bool InputVirtual::isButtonDown(InputButton code)
{
	for (auto& bind : buttons.at(code)) {
		if (bind.isButtonDown()) {
			return true;
		}
	}
	return false;
}

bool InputVirtual::isButtonPressed(InputButton code, gsl::span<const uint32_t> activeBinds)
{
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

float InputVirtual::getAxis(int n)
{
	if (n < 0 || n >= axes.size()) {
		return 0;
	}
	return axes.at(n).getValue();
}

int InputVirtual::getAxisRepeat(int n)
{
	if (n < 0 || n >= axes.size()) {
		return 0;
	}
	return axes.at(n).curRepeatValue;
}

void InputVirtual::bindButton(ConvertibleTo<int> n, spInputDevice device, ConvertibleTo<int> deviceN)
{
	if (!lastDevice.lock()) {
		setLastDevice(device);
	}
	buttons.at(n.value).push_back(Bind(std::move(device), deviceN.value, -1, false));
	exclusiveDirty = true;
}

void InputVirtual::bindButton(ConvertibleTo<int> n, spInputDevice device, KeyCode deviceButton, std::optional<KeyMods> mods)
{
	if (!lastDevice.lock()) {
		setLastDevice(device);
	}
	buttons.at(n.value).push_back(Bind(std::move(device), static_cast<int>(deviceButton), -1, false, mods));
	exclusiveDirty = true;
}

void InputVirtual::bindButtonChord(ConvertibleTo<int> n, spInputDevice device, ConvertibleTo<int> deviceButton0, ConvertibleTo<int> deviceButton1)
{
	if (!lastDevice.lock()) {
		setLastDevice(device);
	}
	buttons.at(n.value).push_back(Bind(std::move(device), deviceButton0.value, deviceButton1.value, false));
	exclusiveDirty = true;
}

void InputVirtual::bindAxis(ConvertibleTo<int> n, spInputDevice device, ConvertibleTo<int> deviceN, float scale)
{
	if (!lastDevice.lock()) {
		setLastDevice(device);
	}
	axes.at(n.value).binds.push_back(Bind(std::move(device), deviceN.value, -1, true, {}, scale));
	exclusiveDirty = true;
}

void InputVirtual::bindAxisButton(ConvertibleTo<int> n, spInputDevice device, ConvertibleTo<int> negativeButton, ConvertibleTo<int> positiveButton)
{
	if (!lastDevice.lock()) {
		setLastDevice(device);
	}
	axes.at(n.value).binds.push_back(Bind(std::move(device), negativeButton.value, positiveButton.value, true));
	exclusiveDirty = true;
}

void InputVirtual::bindAxisButton(ConvertibleTo<int> n, spInputDevice device, KeyCode negativeButton, KeyCode positiveButton, std::optional<KeyMods> mods)
{
	if (!lastDevice.lock()) {
		setLastDevice(device);
	}
	axes.at(n.value).binds.push_back(Bind(std::move(device), static_cast<int>(negativeButton), static_cast<int>(positiveButton), true, mods));
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

String InputVirtual::getButtonName(int code) const
{
	const auto& binds = buttons.at(code);
	if (!binds.empty()) {
		for (const auto& bind: binds) {
			if (bind.device == lastDevice.lock()) {
				return bind.device->getButtonName(bind.a);
			}
		}
		return binds.front().device->getButtonName(binds.front().a);
	} else {
		return "-";
	}
}

void InputVirtual::update(Time t)
{
	updateLastDevice();

	for (auto& axis: axes) {
		axis.curRepeatValue = axis.repeat.update(axis.getValue(), t);
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
	return lastDevice.lock().get();
}

void InputVirtual::updateLastDevice()
{
	if (!lastDeviceFrozen) {
		for (auto& buttonBinds: buttons) {
			for (auto& bind: buttonBinds) {
				if (bind.device && !bind.device->isManual()) {
					if (!bind.isAxisEmulation && bind.device->isButtonPressed(bind.a)) {
						setLastDevice(bind.device);
						return;
					}
				}
			}
		}
		for (auto& axisBind: axes) {
			for (auto& bind: axisBind.binds) {
				if (bind.device && !bind.device->isManual()) {
					if ((!bind.isAxisEmulation && fabs(bind.device->getAxis(bind.a)) > 0.1f)
						|| (bind.isAxisEmulation && bind.device->isButtonDown(bind.a))
						|| (bind.isAxisEmulation && bind.device->isButtonDown(bind.b))) {
						setLastDevice(bind.device);
						return;
					}
				}
			}
		}
	}
}

InputVirtual::Bind::Bind(spInputDevice d, int a, int b, bool axis, std::optional<KeyMods> mods, float scale)
	: device(std::move(d))
	, a(a)
	, b(b)
	, isAxis(axis)
	, isAxisEmulation(axis && b != -1)
	, mods(mods)
	, scale(scale)
{}

bool InputVirtual::Bind::isButtonPressed() const
{
	if (!checkMods()) {
		return false;
	}
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
	return false;
}

bool InputVirtual::Bind::isButtonPressedRepeat() const
{
	if (!checkMods()) {
		return false;
	}
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
	return false;
}

bool InputVirtual::Bind::isButtonReleased() const
{
	if (!checkMods()) {
		return false;
	}
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
	return false;
}

bool InputVirtual::Bind::isButtonDown() const
{
	if (!checkMods()) {
		return false;
	}
	if (b == -1) {
		if (device->isButtonDown(a)) {
			return true;
		}
	} else {
		if (device->isButtonDown(a) && device->isButtonDown(b)) {
			return true;
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
	if (isAxisEmulation) {
		const int left = device->isButtonDown(a) ? 1 : 0;
		const int right = device->isButtonDown(b) ? 1 : 0;
		return static_cast<float>(right - left) * scale;
	} else {
		return device->getAxis(a) * scale;
	}
}

std::pair<uint32_t, uint32_t> InputVirtual::Bind::getPhysicalButtonIds() const
{
	if (isAxis && !isAxisEmulation) {
		const auto idA = (static_cast<uint32_t>(device->getId()) << 16) | static_cast<uint32_t>(0x100) | static_cast<uint32_t>(a);
		return { idA, 0 };
	} else {
		const auto idA = (static_cast<uint32_t>(device->getId()) << 16) | static_cast<uint32_t>(a);
		if (b != -1) {
			const auto idB = (static_cast<uint32_t>(device->getId()) << 16) | static_cast<uint32_t>(b);
			assert(idB != 0); // If this is ever zero, it will conflict with "empty" id below
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

JoystickType InputVirtual::getJoystickType() const
{
	if (auto ld = lastDevice.lock()) {
		return ld->getJoystickType();
	} else {
		return JoystickType::None;
	}
}

InputType InputVirtual::getInputType() const
{
	return type;
}

void InputVirtual::setLastDevice(const std::shared_ptr<InputDevice>& device)
{
	const auto parent = device->getParent();
	if (parent) {
		setLastDevice(parent);
	} else {
		lastDevice = device;
	}
}

std::unique_ptr<InputExclusiveButton> InputVirtual::makeExclusiveButton(InputButton button, InputPriority priority, const InputLabel& label)
{
	auto exclusive = std::make_unique<InputExclusiveButton>(*this, priority, button, label);
	addExclusiveButton(*exclusive);
	return exclusive;
}

std::unique_ptr<InputExclusiveAxis> InputVirtual::makeExclusiveAxis(int axis, InputPriority priority, const InputLabel& label)
{
	auto exclusive = std::make_unique<InputExclusiveAxis>(*this, priority, axis, label);
	addExclusiveAxis(*exclusive);
	return exclusive;
}

Vector<InputVirtual::ExclusiveButtonInfo> InputVirtual::getExclusiveButtonLabels(InputDevice* preferredDevice)
{
	refreshExclusives();

	if (!preferredDevice) {
		preferredDevice = lastDevice.lock().get();
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
		device = lastDevice.lock().get();
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

std::pair<InputDevice*, int> InputVirtual::getPhysicalButton(const InputExclusiveButton& button, InputDevice* device) const
{
	if (!device) {
		device = lastDevice.lock().get();
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

		HashMap<uint32_t, Vector<InputExclusiveBinding*>> bindings;
		
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
