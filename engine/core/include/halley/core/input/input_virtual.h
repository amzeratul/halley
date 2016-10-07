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

namespace Halley {
	using spInputDevice = std::shared_ptr<InputDevice>;

	class InputVirtual : public InputDevice {
	public:
		InputVirtual(int nButtons, int nAxes);
		
		bool isEnabled() const override { return true; }
		virtual size_t getNumberHats() override { return 0; }
		virtual std::shared_ptr<InputDevice> getHat(int /*n*/) override { return std::shared_ptr<InputDevice>(); }

		size_t getNumberButtons() override;
		size_t getNumberAxes() override;

		bool isAnyButtonPressed() override;
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

		void bindButton(int n, spInputDevice device, int deviceN);
		void bindAxis(int n, spInputDevice device, int deviceN);
		void bindAxisButton(int n, spInputDevice device, int negativeButton, int positiveButton);
		void bindVibrationOverride(spInputDevice joy);
		void bindHat(int leftRight, int upDown, spInputDevice hat);

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

			Bind(spInputDevice d, int n) : device(d), a(n), b(0), isAxis(true) {}
			Bind(spInputDevice d, int _a, int _b) : device(d), a(_a), b(_b), isAxis(false) {}
		};

		struct AxisData {
			Vector<Bind> binds;
			float lastValue;
			Time timeout;

			AxisData() : lastValue(0), timeout(0) {}
			AxisData(Vector<Bind>& b) : binds(b), lastValue(0), timeout(0) {}
		};

		Vector<Vector<Bind> > buttons;
		Vector<AxisData> axes;
		spInputDevice vibrationOverride;
		spInputDevice lastDevice;
		bool lastDeviceFrozen;
		
		float repeatDelayFirst;
		float repeatDelayHold;
	};

	typedef std::shared_ptr<InputVirtual> spInputVirtual;
}
