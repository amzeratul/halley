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

using namespace Halley;

InputVirtual::InputVirtual(int nButtons, int nAxes)
	: lastDeviceFrozen(false)
	, repeatDelayFirst(0.20f)
	, repeatDelayHold(0.10f)
{
	buttons.resize(nButtons);
	axes.resize(nAxes);
}

bool InputVirtual::isEnabled() const
{
	for (auto& d: getAllDevices()) {
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
	for (size_t j=0; j < buttons.size(); j++) {
		auto& binds = buttons[j];
		for (size_t i=0; i<binds.size(); i++) {
			Bind& bind = binds[i];
			if (bind.device->isAnyButtonPressed()) {
				return true;
			}
		}
	}
	return false;
}

bool InputVirtual::isAnyButtonReleased()
{
	for (size_t j=0; j < buttons.size(); j++) {
		auto& binds = buttons[j];
		for (size_t i=0; i<binds.size(); i++) {
			Bind& bind = binds[i];
			if (bind.device->isAnyButtonReleased()) {
				return true;
			}
		}
	}
	return false;
}

bool InputVirtual::isAnyButtonDown()
{
	for (size_t j=0; j < buttons.size(); j++) {
		auto& binds = buttons[j];
		for (size_t i=0; i<binds.size(); i++) {
			Bind& bind = binds[i];
			if (bind.device->isAnyButtonDown()) {
				return true;
			}
		}
	}
	return false;
}

bool InputVirtual::isButtonPressed(int code)
{
	auto& binds = buttons.at(code);
	for (size_t i=0; i<binds.size(); i++) {
		Bind& bind = binds[i];
		if (bind.device->isButtonPressed(bind.a)) {
			return true;
		}
	}
	return false;
}

bool InputVirtual::isButtonPressedRepeat(int code)
{
	auto& binds = buttons.at(code);
	for (size_t i=0; i<binds.size(); i++) {
		Bind& bind = binds[i];
		if (bind.device->isButtonPressedRepeat(bind.a)) {
			return true;
		}
	}
	return false;
}

bool InputVirtual::isButtonReleased(int code)
{
	auto& binds = buttons.at(code);
	for (size_t i=0; i<binds.size(); i++) {
		Bind& bind = binds[i];
		if (bind.device->isButtonReleased(bind.a)) {
			return true;
		}
	}
	return false;
}

bool InputVirtual::isButtonDown(int code)
{
	auto& binds = buttons.at(code);
	for (size_t i=0; i<binds.size(); i++) {
		Bind& bind = binds[i];
		if (bind.device->isButtonDown(bind.a)) {
			return true;
		}
	}
	return false;
}

void InputVirtual::clearButton(int code)
{
	auto& binds = buttons.at(code);
	for (size_t i=0; i<binds.size(); i++) {
		Bind& bind = binds[i];
		bind.device->clearButton(bind.a);
	}
}

void InputVirtual::clearButtonPress(int code)
{
	auto& binds = buttons.at(code);
	for (size_t i=0; i<binds.size(); i++) {
		Bind& bind = binds[i];
		bind.device->clearButtonPress(bind.a);
	}
}

void InputVirtual::clearButtonRelease(int code)
{
	auto& binds = buttons.at(code);
	for (size_t i=0; i<binds.size(); i++) {
		Bind& bind = binds[i];
		bind.device->clearButtonRelease(bind.a);
	}
}

float InputVirtual::getAxis(int n)
{
	auto& binds = axes.at(n).binds;
	float value = 0;

	for (size_t i=0; i<binds.size(); i++) {
		Bind& b = binds[i];
		if (b.isAxisEmulation) {
			int left = b.device->isButtonDown(b.a) ? 1 : 0;
			int right = b.device->isButtonDown(b.b) ? 1 : 0;
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
	buttons.at(n).push_back(Bind(device, deviceN, false));
}

void InputVirtual::bindAxis(int n, spInputDevice device, int deviceN)
{
	if (!lastDevice) {
		setLastDevice(device.get());
	}
	axes.at(n).binds.push_back(Bind(device, deviceN, true));
}

void InputVirtual::bindAxisButton(int n, spInputDevice device, int negativeButton, int positiveButton)
{
	if (!lastDevice) {
		setLastDevice(device.get());
	}
	axes.at(n).binds.push_back(Bind(device, negativeButton, positiveButton, true));
}

void InputVirtual::bindVibrationOverride(spInputDevice joy)
{
	vibrationOverride = joy;
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
	for (size_t i=0; i<buttons.size(); i++) {
		buttons[i].clear();
	}
	for (size_t i=0; i<axes.size(); i++) {
		axes[i].binds.clear();
	}
	vibrationOverride = spInputDevice();
}

void InputVirtual::vibrate(spInputVibration vib)
{
	auto dev = vibrationOverride ? vibrationOverride.get() : lastDevice;
	if (dev) {
		dev->vibrate(vib);
	}
}

void InputVirtual::stopVibrating()
{
	auto dev = vibrationOverride ? vibrationOverride.get() : lastDevice;
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
	for (auto& w: wheels) {
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
	positions.push_back(PositionBindData(device));
}

void InputVirtual::bindPositionRelative(spInputDevice device, int axisX, int axisY, float speed)
{
	positions.push_back(PositionBindData(device, axisX, axisY, speed));
}

void InputVirtual::bindWheel(spInputDevice device)
{
	wheels.push_back(device);
}

String InputVirtual::getButtonName(int code)
{
	auto& binds = buttons.at(code);
	if (binds.size() > 0) {
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
		int intVal = curVal > 0.30f ? 1 : (curVal < -0.30f ? -1 : 0);

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
		position = positionLimits.get().getClosestPoint(position);
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
	: device(d)
	, a(n)
	, b(0)
	, isAxis(axis)
	, isAxisEmulation(false)
{}

InputVirtual::Bind::Bind(spInputDevice d, int _a, int _b, bool axis)
	: device(d)
	, a(_a)
	, b(_b)
	, isAxis(axis)
	, isAxisEmulation(true)
{}

InputVirtual::AxisData::AxisData()
{}

InputVirtual::AxisData::AxisData(Vector<Bind>& b)
	: binds(b)
{}

InputVirtual::PositionBindData::PositionBindData()
{}

InputVirtual::PositionBindData::PositionBindData(spInputDevice device)
	: device(device)
	, direct(true)
{}

InputVirtual::PositionBindData::PositionBindData(spInputDevice device, int axisX, int axisY, float speed)
	: device(device)
	, axisX(axisX)
	, axisY(axisY)
	, speed(speed)
{}

std::set<spInputDevice> InputVirtual::getAllDevices() const
{
	std::set<spInputDevice> devices;

	for (auto& axisBind: axes) {
		for (auto& bind: axisBind.binds) {
			if (bind.device) {
				devices.insert(bind.device);
			}
		}
	}
	for (auto& buttonBinds: buttons) {
		for (auto& bind: buttonBinds) {
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
	auto parent = device->getParent();
	if (parent) {
		setLastDevice(parent);
	} else {
		lastDevice = device;
	}
}
