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

#include "input_keyboard.h"
#include "input_joystick.h"
#include "input_mouse.h"
#include "input_virtual.h"
#include "input_manual.h"
#include "input_touch.h"

namespace Halley {

	class Input {
	public:
		spInputKeyboard getKeyboard(int id=0);
		spInputJoystick getJoystick(int id=0);
		spInputJoystick getFirstJoystick();
		spInputMouse getMouse(int id=0);
		size_t getNumberOfKeyboards();
		size_t getNumberOfJoysticks();
		size_t getNumberOfMice();

		std::vector<spInputTouch> getNewTouchEvents();
		std::vector<spInputTouch> getTouchEvents();

		void beginEvents(Time t);
		void processEvent(SDL_Event& event);

	private:
		Input();

		void processJoyEvent(int n, SDL_Event& event);
		void processTouch(int type, long long touchId, long long fingerId, float x, float y);
		
		std::vector<spInputKeyboardConcrete> keyboards;
		std::vector<spInputJoystick> joysticks;
		std::vector<spInputMouseConcrete> mice;

		std::map<int, spInputJoystick> sdlJoys;
		std::map<int, spInputTouch> touchEvents;
	};

};
