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

#include "halley/core/input/input_joystick.h"

namespace Halley {

	class InputJoystickSDL final : public InputJoystick {
	public:
		~InputJoystickSDL();
		std::string getName() const final override;

		void update(Time t) override;

		JoystickType getJoystickType() const override {
			return JoystickType::Generic;
		}

		virtual int getButtonAtPosition(JoystickButtonPosition position) const override;

	private:
		InputJoystickSDL(int number);
		int getSDLAxisIndex(int axis);
		void processAxisEvent(int axis, float value);
		void processEvent(const SDL_Event& event);

		void* joystick;
		int index;

		friend class InputSDL;
	};

	class InputJoystickHatSDL : public InputButtonBase {
	private:
		InputJoystickHatSDL() : InputButtonBase(4) {}

		void processEvent(const SDL_Event& event);
	
		friend class InputJoystickSDL;
	};

}
