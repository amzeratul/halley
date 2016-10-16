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
	, repeatDelayFirst(0.3f)
	, repeatDelayHold(0.15f)
{
	buttons.resize(nButtons);
	axes.resize(nAxes);
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
		};
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
		};
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
	};
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
	};
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

void Halley::InputVirtual::clearButton(int code)
{
	auto& binds = buttons.at(code);
	for (size_t i=0; i<binds.size(); i++) {
		Bind& bind = binds[i];
		bind.device->clearButton(bind.a);
	}
}

float InputVirtual::getAxis(int n)
{
	auto& binds = axes.at(n).binds;
	float value = 0;

	for (size_t i=0; i<binds.size(); i++) {
		Bind& b = binds[i];
		if (b.isAxis) {
			value += b.device->getAxis(b.a);
		} else {
			int left = b.device->isButtonDown(b.a) ? 1 : 0;
			int right = b.device->isButtonDown(b.b) ? 1 : 0;
			value += right - left;
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
	buttons.at(n).push_back(Bind(device, deviceN));
}

void InputVirtual::bindAxis(int n, spInputDevice device, int deviceN)
{
	axes.at(n).binds.push_back(Bind(device, deviceN));
}

void InputVirtual::bindAxisButton(int n, spInputDevice device, int negativeButton, int positiveButton)
{
	axes.at(n).binds.push_back(Bind(device, negativeButton, positiveButton));
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

void Halley::InputVirtual::vibrate(spInputVibration vib)
{
	spInputDevice dev = vibrationOverride ? vibrationOverride : lastDevice;
	if (dev) {
		dev->vibrate(vib);
	}
}

void Halley::InputVirtual::stopVibrating()
{
	spInputDevice dev = vibrationOverride ? vibrationOverride : lastDevice;
	if (dev) {
		dev->stopVibrating();
	}
}

void Halley::InputVirtual::bindHat(int leftRight, int upDown, spInputDevice hat)
{
	bindAxisButton(leftRight, hat, 3, 1);
	bindAxisButton(upDown, hat, 0, 2);
}

Halley::String Halley::InputVirtual::getButtonName(int code)
{
	auto& binds = buttons.at(code);
	if (binds.size() > 0) {
		Bind& bind = binds[0];
		return bind.device->getButtonName(bind.a);
	} else {
		return "-";
	}
}

void Halley::InputVirtual::update(Time t)
{
	updateLastDevice();

	for (size_t i = 0; i < axes.size(); i++) {
		auto& axis = axes[i];

		axis.curRepeatValue = 0;

		float curVal = getAxis(int(i));
		int intVal = curVal > 0.50f ? 1 : (curVal < -0.50f ? - 1 : 0);

		auto& timeSinceRepeat = axis.timeSinceRepeat;
		auto& lastVal = axis.lastRepeatedValue;

		if (lastVal != intVal) {
			timeSinceRepeat = std::max(repeatDelayFirst, repeatDelayHold);
			axis.numRepeats = 0;
		}
		
		if (intVal != 0) {
			Time threshold = axis.numRepeats == 1 ? repeatDelayFirst : repeatDelayHold;
			timeSinceRepeat += t;
			if (timeSinceRepeat >= threshold) {
				axis.curRepeatValue = intVal;
				axis.numRepeats++;
				timeSinceRepeat = 0;
			}
		}

		lastVal = intVal;
	}
}

Halley::spInputDevice Halley::InputVirtual::getLastDevice() const
{
	return lastDevice;
}

void Halley::InputVirtual::updateLastDevice()
{
	if (!lastDeviceFrozen) {
		for (size_t code=0; code<buttons.size(); code++) {
			auto& binds = buttons[code];
			for (size_t i=0; i<binds.size(); i++) {
				Bind& bind = binds[i];
				if (bind.device->isButtonPressed(bind.a)) {
					if (!std::dynamic_pointer_cast<InputManual>(bind.device)) {
						lastDevice = bind.device;
						return;
					}
				}
			}
		}
	}
}

InputVirtual::Bind::Bind(spInputDevice d, int n): device(d), a(n), b(0), isAxis(true) {}

InputVirtual::Bind::Bind(spInputDevice d, int _a, int _b): device(d), a(_a), b(_b), isAxis(false) {}

InputVirtual::AxisData::AxisData()
{}

InputVirtual::AxisData::AxisData(Vector<Bind>& b)
	: binds(b)
{}

void Halley::InputVirtual::setLastDeviceFreeze(bool frozen)
{
	lastDeviceFrozen = frozen;
}

void Halley::InputVirtual::setRepeat(float first, float hold)
{
	repeatDelayFirst = first;
	repeatDelayHold = hold;
}
