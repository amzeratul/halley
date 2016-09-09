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

#include "halley/core/api/halley_api_internal.h"
#include <halley/data_structures/flat_map.h>
#include <SDL.h>
#include "input_joystick_sdl.h"

namespace Halley {

	class InputKeyboardSDL;
	class InputMouseSDL;

	class InputSDL final : public InputAPIInternal {
		friend class HalleyAPI;

	public:
		InputSDL();
		~InputSDL();

		size_t getNumberOfKeyboards() const override;
		std::shared_ptr<InputKeyboard> getKeyboard(int id=0) const override;

		size_t getNumberOfJoysticks() const override;
		std::shared_ptr<InputJoystick> getJoystick(int id=0) const override;

		size_t getNumberOfMice() const override;
		std::shared_ptr<InputMouse> getMouse(int id=0) const override;

		Vector<std::shared_ptr<InputTouch>> getNewTouchEvents() override;
		Vector<std::shared_ptr<InputTouch>> getTouchEvents() override;

		void processEvent(SDL_Event& event);

	private:
		void init() override;
		void deInit() override;
		void beginEvents(Time t) override;

		void processJoyEvent(int n, SDL_Event& event);
		void processTouch(int type, long long touchId, long long fingerId, float x, float y);
		
		Vector<std::shared_ptr<InputKeyboardSDL>> keyboards;
		Vector<std::shared_ptr<InputJoystick>> joysticks;
		Vector<std::shared_ptr<InputMouseSDL>> mice;

		FlatMap<int, InputJoystickSDL*> sdlJoys;
		FlatMap<int, std::shared_ptr<InputTouch>> touchEvents;
	};

};
