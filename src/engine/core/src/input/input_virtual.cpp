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

#include "input/input_virtual.h"
#include "input/input_manual.h"
#include <set>
#include <algorithm>
#include <utility>

#include "halley/utils/algorithm.h"
#include "input/input_exclusive.h"

using namespace Halley;

InputVirtual::InputVirtual(int nButtons, int nAxes)
	: lastDeviceFrozen(false)
	, repeatDelayFirst(0.20f)
	, repeatDelayHold(0.10f)
{
	buttons.resize(nButtons);
	axes.resize(nAxes);
}

InputVirtual::~InputVirtual()
{
	for (auto& binding: exclusiveButtons) {
		for (auto& button: binding.second) {
			button->parent = nullptr;
			button->active = false;
		}
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
	auto& binds = buttons.at(code);
	for (auto& bind : binds) {
		if (bind.b == -1) {
			// Single bind
			if (bind.device->isButtonPressed(bind.a)) {
				return true;
			}
		} else {
			// Chord bind
			if ((bind.device->isButtonPressed(bind.a) && bind.device->isButtonDown(bind.b)) || (bind.device->isButtonPressed(bind.b) && bind.device->isButtonDown(bind.a))) {
				return true;
			}
		}
	}
	return false;
}

bool InputVirtual::isButtonPressedRepeat(InputButton code)
{
	auto& binds = buttons.at(code);
	for (auto& bind : binds) {
		if (bind.b == -1) {
			// Single bind
			if (bind.device->isButtonPressedRepeat(bind.a)) {
				return true;
			}
		} else {
			// Chord bind
			if (bind.device->isButtonPressedRepeat(bind.a) && bind.device->isButtonDown(bind.b)) {
				return true;
			}
		}
	}
	return false;
}

bool InputVirtual::isButtonReleased(InputButton code)
{
	auto& binds = buttons.at(code);
	for (auto& bind : binds) {
		if (bind.b == -1) {
			// Single bind
			if (bind.device->isButtonReleased(bind.a)) {
				return true;
			}
		} else {
			// Chord bind
			const bool aReleased = bind.device->isButtonReleased(bind.a);
			const bool bReleased = bind.device->isButtonReleased(bind.b);
			if ((aReleased && bReleased) || (aReleased && bind.device->isButtonDown(bind.b)) || (bReleased && bind.device->isButtonDown(bind.a))) {
				return true;
			}
		}
	}
	return false;
}

bool InputVirtual::isButtonDown(InputButton code)
{
	auto& binds = buttons.at(code);
	for (auto& bind : binds) {
		if (bind.b == -1) {
			if (bind.device->isButtonDown(bind.a)) {
				return true;
			}
		} else {
			if (bind.device->isButtonDown(bind.a) && bind.device->isButtonDown(bind.b)) {
				return true;
			}
		}
	}
	return false;
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
	auto& binds = axes.at(n).binds;
	float value = 0;

	for (size_t i=0; i<binds.size(); i++) {
		Bind& b = binds[i];
		if (b.isAxisEmulation) {
			const int left = b.device->isButtonDown(b.a) ? 1 : 0;
			const int right = b.device->isButtonDown(b.b) ? 1 : 0;
			value += right - left;
		} else {
			value += b.device->getAxis(b.a);
		}
	}

	return value;
}

int InputVirtual::getAxisRepeat(int n)
{
	return axes.at(n).curRepeatValue;
}

void InputVirtual::bindButton(int n, spInputDevice device, int deviceN)
{
	if (!lastDevice) {
		setLastDevice(device.get());
	}
	buttons.at(n).push_back(Bind(std::move(device), deviceN, false));
}

void InputVirtual::bindButton(int n, spInputDevice device, KeyCode deviceButton)
{
	bindButton(n, std::move(device), static_cast<int>(deviceButton));
}

void InputVirtual::bindButtonChord(int n, spInputDevice device, int deviceButton0, int deviceButton1)
{
	if (!lastDevice) {
		setLastDevice(device.get());
	}
	buttons.at(n).push_back(Bind(std::move(device), deviceButton0, deviceButton1, false));
}

void InputVirtual::bindAxis(int n, spInputDevice device, int deviceN)
{
	if (!lastDevice) {
		setLastDevice(device.get());
	}
	axes.at(n).binds.push_back(Bind(std::move(device), deviceN, true));
}

void InputVirtual::bindAxisButton(int n, spInputDevice device, int negativeButton, int positiveButton)
{
	if (!lastDevice) {
		setLastDevice(device.get());
	}
	axes.at(n).binds.push_back(Bind(std::move(device), negativeButton, positiveButton, true));
}

void InputVirtual::bindAxisButton(int n, spInputDevice device, KeyCode negativeButton, KeyCode positiveButton)
{
	bindAxisButton(n, std::move(device), static_cast<int>(negativeButton), static_cast<int>(positiveButton));
}

void InputVirtual::bindVibrationOverride(spInputDevice joy)
{
	vibrationOverride = std::move(joy);
}

void InputVirtual::unbindButton(int n)
{
	buttons.at(n).clear();
}

void InputVirtual::unbindAxis(int n)
{
	axes.at(n).binds.clear();
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

void InputVirtual::vibrate(spInputVibration vib)
{
	const auto& dev = vibrationOverride ? vibrationOverride.get() : lastDevice;
	if (dev) {
		dev->vibrate(vib);
	}
}

void InputVirtual::stopVibrating()
{
	const auto& dev = vibrationOverride ? vibrationOverride.get() : lastDevice;
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

int InputVirtual::getWheelMove() const
{
	int val = 0;
	for (const auto& w: wheels) {
		val += w->getWheelMove();
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
	wheels.push_back(std::move(device));
}

String InputVirtual::getButtonName(int code)
{
	auto& binds = buttons.at(code);
	if (!binds.empty()) {
		Bind& bind = binds[0];
		return bind.device->getButtonName(bind.a);
	} else {
		return "-";
	}
}

void InputVirtual::update(Time t)
{
	updateLastDevice();

	for (size_t i = 0; i < axes.size(); i++) {
		auto& axis = axes[i];

		axis.curRepeatValue = 0;

		float curVal = getAxis(int(i));
		int intVal = curVal > 0.50f ? 1 : (curVal < -0.50f ? -1 : 0);

		auto& timeSinceRepeat = axis.timeSinceRepeat;
		auto& lastVal = axis.lastRepeatedValue;

		if (lastVal != intVal) {
			timeSinceRepeat = std::max(repeatDelayFirst, repeatDelayHold);
			axis.numRepeats = 0;
		}
		
		if (intVal != 0) {
			Time threshold = axis.numRepeats == 1 ? repeatDelayFirst : repeatDelayHold;
			timeSinceRepeat += t * fabs(curVal);
			if (timeSinceRepeat >= threshold) {
				axis.curRepeatValue = intVal;
				axis.numRepeats++;
				timeSinceRepeat = 0;
			}
		}

		lastVal = intVal;
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
}

InputDevice* InputVirtual::getLastDevice() const
{
	return lastDevice;
}

void InputVirtual::updateLastDevice()
{
	if (!lastDeviceFrozen) {
		for (auto& buttonBinds: buttons) {
			for (auto& bind: buttonBinds) {
				if (bind.device && !std::dynamic_pointer_cast<InputManual>(bind.device)) {
					if (!bind.isAxisEmulation && bind.device->isButtonPressed(bind.a)) {
						setLastDevice(bind.device.get());
						return;
					}
				}
			}
		}
		for (auto& axisBind: axes) {
			for (auto& bind: axisBind.binds) {
				if (bind.device && !std::dynamic_pointer_cast<InputManual>(bind.device)) {
					if ((!bind.isAxisEmulation && fabs(bind.device->getAxis(bind.a)) > 0.1f)
						|| (bind.isAxisEmulation && bind.device->isButtonDown(bind.a))
						|| (bind.isAxisEmulation && bind.device->isButtonDown(bind.b))) {
						setLastDevice(bind.device.get());
						return;
					}
				}
			}
		}
	}
}

InputVirtual::Bind::Bind(spInputDevice d, int n, bool axis)
	: device(std::move(d))
	, a(n)
	, b(-1)
	, isAxis(axis)
	, isAxisEmulation(false)
{}

InputVirtual::Bind::Bind(spInputDevice d, int _a, int _b, bool axis)
	: device(std::move(d))
	, a(_a)
	, b(_b)
	, isAxis(axis)
	, isAxisEmulation(axis)
{}

InputVirtual::AxisData::AxisData() = default;

InputVirtual::AxisData::AxisData(Vector<Bind> b)
	: binds(std::move(b))
{}

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
	if (lastDevice) {
		return lastDevice->getJoystickType();
	} else {
		return JoystickType::None;
	}
}

void InputVirtual::setLastDevice(InputDevice* device)
{
	const auto parent = device->getParent();
	if (parent) {
		setLastDevice(parent);
	} else {
		lastDevice = device;
	}
}

std::unique_ptr<InputExclusiveButton> InputVirtual::makeExclusiveButton(InputButton button, InputPriority priority, const String& label)
{
	auto exclusive = std::make_unique<InputExclusiveButton>(*this, priority, button, label);
	addExclusiveButton(*exclusive);
	return exclusive;
}

void InputVirtual::clearPresses()
{
	for (auto& axis: axes) {
		axis.curRepeatValue = 0;
	}
}

void InputVirtual::addExclusiveButton(InputExclusiveButton& exclusive)
{
	exclusiveButtons[exclusive.button].push_back(&exclusive);
	refreshButtons(exclusive.button);
}

void InputVirtual::removeExclusiveButton(InputExclusiveButton& exclusive)
{
	std_ex::erase(exclusiveButtons[exclusive.button], &exclusive);
	refreshButtons(exclusive.button);
}

void InputVirtual::refreshButtons(InputButton button)
{
	auto& bs = exclusiveButtons[button];
	if (bs.empty()) {
		exclusiveButtons.erase(button);
	} else {
		std::sort(bs.begin(), bs.end(), [] (const InputExclusiveButton* a, const InputExclusiveButton* b) { return a->priority > b->priority; });
		for (size_t i = 0; i < bs.size(); ++i) {
			bs[i]->active = i == 0;
		}
	}
}
