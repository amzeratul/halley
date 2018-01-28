#pragma once
#include "halley/core/input/input_joystick.h"

namespace Halley
{
	class WinRTGamepad : public InputJoystick {
	public:
		WinRTGamepad(int index);
		~WinRTGamepad();

		std::string getName() const override;
		JoystickType getJoystickType() const override { return JoystickType::Xbox; }
	
		void update(Time t) override;
		int getButtonAtPosition(JoystickButtonPosition position) const override;

	private:
		int index;

		void setVibration(float low, float high) override;
	};
}
