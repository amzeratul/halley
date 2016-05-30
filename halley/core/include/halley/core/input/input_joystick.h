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
#include "input_button_base.h"

#ifdef _MSC_VER
#define XINPUT_AVAILABLE
#endif

namespace Halley {

	enum JoystickType {
		JOYSTICK_GENERIC,
		JOYSTICK_360,
		JOYSTICK_PS3
	};

	class InputJoystick : public InputButtonBase {
	public:
		virtual ~InputJoystick() {}

		virtual std::string getName() const = 0;
		virtual JoystickType getType() const = 0;

		size_t getNumberAxes() override;
		size_t getNumberHats() override;

		float getAxis(int n) override;
		InputDevice& getHat(int n) override;

		static float defaultAxisAdjust(float value);

		int getButtonAtPosition(char c) const;	// c = N, S, E or W

	protected:
		std::vector<float> axes;
		std::vector<spInputButtonBase> hats;
		std::function<float (float)> axisAdjust;

		virtual void update(Time /*t*/) {}
		virtual void processEvent(const SDL_Event& /*event*/) {}

		friend class Input;
	};

	typedef std::shared_ptr<InputJoystick> spInputJoystick;
}
