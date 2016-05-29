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
#include <map>

namespace Halley {

	class InputKeyboardConcrete;
	class InputMouseConcrete;

	class Input : public InputAPIInternal {
		friend class HalleyAPI;

	public:
		size_t getNumberOfKeyboards() const override;
		InputKeyboard& getKeyboard(int id=0) const override;

		size_t getNumberOfJoysticks() const override;
		InputJoystick& getJoystick(int id=0) const override;

		size_t getNumberOfMice() const override;
		InputMouse& getMouse(int id=0) const override;

		std::vector<std::shared_ptr<InputTouch>> getNewTouchEvents() override;
		std::vector<std::shared_ptr<InputTouch>> getTouchEvents() override;

	private:
		Input();
		~Input();

		void init() override;
		void deInit() override;
		void beginEvents(Time t) override;
		void processEvent(SDL_Event& event) override;

		void processJoyEvent(int n, SDL_Event& event);
		void processTouch(int type, long long touchId, long long fingerId, float x, float y);
		
		std::vector<std::unique_ptr<InputKeyboardConcrete>> keyboards;
		std::vector<std::unique_ptr<InputJoystick>> joysticks;
		std::vector<std::unique_ptr<InputMouseConcrete>> mice;

		std::map<int, InputJoystick*> sdlJoys;
		std::map<int, std::shared_ptr<InputTouch>> touchEvents;
	};

};
