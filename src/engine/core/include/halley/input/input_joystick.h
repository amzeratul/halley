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

#if defined(_WIN32) || defined(WITH_GDK)
#define XINPUT_AVAILABLE
#endif

namespace Halley {
	
	class InputJoystick : public InputButtonBase {
	public:
		static float defaultAxisAdjust(float value);

		~InputJoystick() override = default;

		std::string_view getName() const override;
		JoystickType getJoystickType() const override;
		InputType getInputType() const override;

		size_t getNumberAxes() const override;
		size_t getNumberHats() const override;

		float getAxis(int n) const override;
		std::shared_ptr<InputDevice> getHat(int n) const override;

		virtual void update(Time t);

		std::pair<float, float> getVibration() const final;
		void setVibration(float low, float high) final;
		void vibrate(spInputVibration vibration) final;
		void stopVibrating() final;

		bool isEnabled() const override;
		void setEnabled(bool enabled);

		void clearAxes() override;
		void clearPresses() override;

		bool isAnyButtonPressed() const override;
		bool isAnyButtonReleased() const override;
		bool isAnyButtonDown() const override;

		void setAxisAdjust(std::function<float (float)> f);

		String getButtonName(int code) const override;
		String getAxisName(int index) const override;
		static std::string_view getJoystickButtonName(JoystickType type, JoystickButtonPosition position);
		static std::string_view getJoystickAxisName(JoystickType type, JoystickAxisPosition position);
		static String getJoystickAxisName(JoystickType type, JoystickAxisPosition position, JoystickAxisDirection direction);

	protected:
		Vector<float> axes;
		Vector<spInputButtonBase> hats;
		std::function<float (float)> axisAdjust;

		virtual void doSetVibration(float low, float high);

	private:
		Vector<spInputVibration> vibs;
		float baseLowVib = 0;
		float baseHighVib = 0;
		float curLowVib = 0;
		float curHighVib = 0;
		std::chrono::steady_clock::time_point lastTime;
		bool enabled = false;

		void updateVibration(Time t);
	};

	typedef std::shared_ptr<InputJoystick> spInputJoystick;
}
