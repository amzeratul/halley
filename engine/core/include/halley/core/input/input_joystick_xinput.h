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

#include "input_joystick.h"
#ifdef XINPUT_AVAILABLE

namespace Halley {

	// XInput implementation
	class InputJoystickXInput : public InputJoystick {
	public:
		InputJoystickXInput(int number);
		~InputJoystickXInput();

		std::string getName() const override;
		JoystickType getType() const override { return JoystickType::Xbox; }
	
		void update(Time t) override;
		int getButtonAtPosition(JoystickButtonPosition position) const override;

	private:
		int index;
		int cooldown;

		void setVibration(float low, float high) override;
	};
}

#endif
