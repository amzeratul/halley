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

#pragma once

#include "input_button_base.h"
#include "halley/maths/rect.h"
#include "halley/data_structures/maybe.h"
#include <set>

namespace Halley {
	using spInputDevice = std::shared_ptr<InputDevice>;

	class InputVirtual : public InputDevice {
	public:
		InputVirtual(int nButtons, int nAxes);

		bool isEnabled() const override;
		size_t getNumberHats() override;
		std::shared_ptr<InputDevice> getHat(int /*n*/) override;

		size_t getNumberButtons() override;
		size_t getNumberAxes() override;

		bool isAnyButtonPressed() override;
		bool isAnyButtonReleased() override;
		bool isAnyButtonDown() override;

		bool isButtonPressed(int code) override;
		bool isButtonPressedRepeat(int code) override;
		bool isButtonReleased(int code) override;
		bool isButtonDown(int code) override;

		void clearButton(int code) override;
		void clearButtonPress(int code) override;
		void clearButtonRelease(int code) override;

		String getButtonName(int code) override;

		float getAxis(int n) override;
		int getAxisRepeat(int n) override;

		void vibrate(spInputVibration vib) override;
		void stopVibrating() override;

		Vector2f getPosition() const override;
		void setPosition(Vector2f pos);
		void setPositionLimits(Rect4f limits);
		void setPositionLimits();

		int getWheelMove() const override;

		void bindButton(int n, spInputDevice device, int deviceN);
		void bindAxis(int n, spInputDevice device, int deviceN);
		void bindAxisButton(int n, spInputDevice device, int negativeButton, int positiveButton);
		void bindVibrationOverride(spInputDevice joy);
		void bindHat(int leftRight, int upDown, spInputDevice hat);
		void bindPosition(spInputDevice device);
		void bindPositionRelative(spInputDevice device, int axisX, int axisY, float speed);
		void bindWheel(spInputDevice device);

		void unbindButton(int n);
		void unbindAxis(int n);
		void clearBindings();

		void update(Time t);

		void setRepeat(float first, float hold);

		InputDevice* getLastDevice() const;
		void setLastDeviceFreeze(bool frozen);

		JoystickType getJoystickType() const override;

	private:
		void setLastDevice(InputDevice* device);
		void updateLastDevice();

		struct Bind {
			spInputDevice device;
			int a = -1;
			int b = -1;
			bool isAxis = false;
			bool isAxisEmulation = false;

			Bind(spInputDevice d, int n, bool axis);
			Bind(spInputDevice d, int _a, int _b, bool axis);
		};

		struct AxisData {
			Vector<Bind> binds;
			int lastRepeatedValue = 0;
			int numRepeats = 0;
			int curRepeatValue = 0;
			Time timeSinceRepeat = 0;

			AxisData();
			explicit AxisData(Vector<Bind>& b);
		};

		struct PositionBindData
		{
			spInputDevice device;
			bool direct = false;
			int axisX = 0;
			int axisY = 0;
			float speed = 0.0f;
			Vector2f lastRead;

			PositionBindData();
			explicit PositionBindData(spInputDevice device);
			explicit PositionBindData(spInputDevice device, int axisX, int axisY, float speed);
		};

		Vector<Vector<Bind>> buttons;
		Vector<AxisData> axes;

		Vector<PositionBindData> positions;
		std::optional<Rect4f> positionLimits;
		Vector2f position;

		Vector<spInputDevice> wheels;

		spInputDevice vibrationOverride;
		InputDevice* lastDevice = nullptr;
		bool lastDeviceFrozen;
		
		float repeatDelayFirst;
		float repeatDelayHold;

		std::set<spInputDevice> getAllDevices() const;
	};

	typedef std::shared_ptr<InputVirtual> spInputVirtual;
}
