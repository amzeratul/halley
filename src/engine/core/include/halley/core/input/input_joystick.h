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

#include <functional>
#include <chrono>
#include "input_button_base.h"

#if defined(_WIN32) && !defined(WINDOWS_STORE)
#define XINPUT_AVAILABLE
#endif

namespace Halley {

	enum class JoystickButtonPosition
	{
		FaceTop,
		FaceRight,
		FaceBottom,
		FaceLeft,
		BumperLeft,
		BumperRight,
		TriggerLeft,
		TriggerRight,
		LeftStick,
		RightStick,
		Select,
		Start,
		PlatformAcceptButton,
		PlatformCancelButton
	};

	class InputJoystick : public InputButtonBase {
	public:
		virtual ~InputJoystick() {}

		virtual std::string getName() const = 0;
		JoystickType getJoystickType() const override;

		size_t getNumberAxes() override;
		size_t getNumberHats() override;

		float getAxis(int n) override;
		std::shared_ptr<InputDevice> getHat(int n) override;

		static float defaultAxisAdjust(float value);

		virtual int getButtonAtPosition(JoystickButtonPosition position) const = 0;

		virtual void update(Time t);

		void vibrate(spInputVibration vibration) override;
		void stopVibrating() override;

		bool isEnabled() const override;
		void setEnabled(bool enabled);

		void clearAxes();

		bool isAnyButtonPressed() override;
		bool isAnyButtonReleased() override;
		bool isAnyButtonDown() override;

		void setAxisAdjust(std::function<float (float)> f);

	protected:
		Vector<float> axes;
		Vector<spInputButtonBase> hats;
		std::function<float (float)> axisAdjust;

		virtual void setVibration(float low, float high);

	private:
		Vector<spInputVibration> vibs;
		float curLowVib = 0;
		float curHighVib = 0;
		std::chrono::steady_clock::time_point lastTime;
		bool enabled = false;

		void updateVibration(Time t);
	};

	typedef std::shared_ptr<InputJoystick> spInputJoystick;
}
