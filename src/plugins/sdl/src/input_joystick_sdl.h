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

#include "halley/input/input_joystick.h"

namespace Halley {

	class InputJoystickSDL final : public InputJoystick {
	public:
		~InputJoystickSDL();

		void update(Time t) override;
		void close();

		std::string_view getName() const final override;
		JoystickType getJoystickType() const override { return JoystickType::Generic; }
		int getSDLJoystickId() const;

		int getButtonAtPosition(JoystickButtonPosition position) const override;

	private:
		InputJoystickSDL(int number);
		int getSDLAxisIndex(int axis);
		void processAxisEvent(int axis, float value);
		void processEvent(const SDL_Event& event);

		void doSetVibration(float low, float high) override;

		void* joystick = nullptr;
		int index = 0;
		int id = 0;
		int baseButtons = 0;
		String name;

		friend class InputSDL;
	};

	class InputJoystickHatSDL : public InputButtonBase {
	private:
		InputJoystickHatSDL() : InputButtonBase(4) {}

		void processEvent(const SDL_Event& event);
	
		friend class InputJoystickSDL;
	};

}
