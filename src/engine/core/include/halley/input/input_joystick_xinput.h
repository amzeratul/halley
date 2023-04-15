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
	class InputJoystickXInput : public InputJoystick, public std::enable_shared_from_this<InputJoystickXInput> {
	public:
		InputJoystickXInput(int number);
		~InputJoystickXInput() override;

		std::string_view getName() const override;
		JoystickType getJoystickType() const override { return JoystickType::Xbox; }
		InputType getInputType() const override;
	
		void update(Time t) override;
		int getButtonAtPosition(JoystickButtonPosition position) const override;
		String getButtonName(int code) const override;

	private:
		int index;
		int cooldown;
		mutable String name;

		void doSetVibration(float low, float high) override;
	};
}

#endif
