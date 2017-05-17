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

		bool isButtonPressed(int code) override;
		bool isButtonPressedRepeat(int code) override;
		bool isButtonReleased(int code) override;
		bool isButtonDown(int code) override;

		void clearButton(int code) override;

		String getButtonName(int code) override;

		float getAxis(int n) override;
		int getAxisRepeat(int n) override;

		void vibrate(spInputVibration vib) override;
		void stopVibrating() override;

		Vector2f getPosition() const override;
		void setPosition(Vector2f pos);
		void setPositionLimits(Rect4f limits);
		void setPositionLimits();

		void bindButton(int n, spInputDevice device, int deviceN);
		void bindAxis(int n, spInputDevice device, int deviceN);
		void bindAxisButton(int n, spInputDevice device, int negativeButton, int positiveButton);
		void bindVibrationOverride(spInputDevice joy);
		void bindHat(int leftRight, int upDown, spInputDevice hat);
		void bindPosition(spInputDevice device);
		void bindPositionRelative(spInputDevice device, int axisX, int axisY, float speed);

		void unbindButton(int n);
		void unbindAxis(int n);
		void clearBindings();

		void update(Time t);

		void setRepeat(float first, float hold);

		spInputDevice getLastDevice() const;
		void setLastDeviceFreeze(bool frozen);

	private:
		void updateLastDevice();

		struct Bind {
			spInputDevice device;
			int a;
			int b;
			bool isAxis;

			Bind(spInputDevice d, int n);
			Bind(spInputDevice d, int _a, int _b);
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
		Maybe<Rect4f> positionLimits;
		Vector2f position;

		spInputDevice vibrationOverride;
		spInputDevice lastDevice;
		bool lastDeviceFrozen;
		
		float repeatDelayFirst;
		float repeatDelayHold;
	};

	typedef std::shared_ptr<InputVirtual> spInputVirtual;
}
