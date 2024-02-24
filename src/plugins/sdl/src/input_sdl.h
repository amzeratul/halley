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

#include "halley/api/halley_api_internal.h"
#include <map>
#include <SDL.h>
#include "input_joystick_sdl.h"

namespace Halley {
	class InputGameControllerSDL;

	class InputKeyboardSDL;
	class InputMouseSDL;
	class SystemSDL;

	class InputSDL final : public InputAPIInternal {
		friend class HalleyAPI;

	public:
		explicit InputSDL(SystemAPI& system);
		~InputSDL();

		void setResources(Resources& resources) override;

		size_t getNumberOfKeyboards() const override;
		std::shared_ptr<InputKeyboard> getKeyboard(int id) const override;

		size_t getNumberOfJoysticks() const override;
		std::shared_ptr<InputDevice> getJoystick(int id) const override;

		size_t getNumberOfMice() const override;
		std::shared_ptr<InputDevice> getMouse(int id) const override;

		Vector<std::shared_ptr<InputTouch>> getNewTouchEvents() override;
		Vector<std::shared_ptr<InputTouch>> getTouchEvents() override;

		void setMouseTrap(bool shouldBeTrapped) override;
		void setMouseCursorPos(Vector2i pos) override;
		void setMouseCursorMode(std::optional<MouseCursorMode> mode) override;

		void processEvent(SDL_Event& event);

		void setMouseRemapping(std::function<Vector2f(Vector2i)> remapFunction) override;

	private:
		void init() override;
		void deInit() override;
		void beginEvents(Time t) override;

		void processJoyEvent(int n, const SDL_Event& event);
		void processJoyDeviceEvent(const SDL_JoyDeviceEvent& event);
		void processGameControllerEvent(int n, const SDL_Event& event);
		void processGameControllerDeviceEvent(const SDL_ControllerDeviceEvent& event);
		void processTouch(int type, long long touchId, long long fingerId, float x, float y);

		void addJoystick(int idx);

		SystemSDL& system;
		
		Vector<std::shared_ptr<InputKeyboardSDL>> keyboards;
		Vector<std::shared_ptr<InputJoystick>> joysticks;
		Vector<std::shared_ptr<InputMouseSDL>> mice;

		HashMap<int, std::shared_ptr<InputJoystickSDL>> sdlJoys;
		HashMap<int, std::shared_ptr<InputGameControllerSDL>> sdlGameControllers;
		HashMap<int, std::shared_ptr<InputTouch>> touchEvents;
		HashMap<SDL_SystemCursor, SDL_Cursor*> cursors;
		std::optional<MouseCursorMode> curCursor;

		std::function<Vector2f(Vector2i)> mouseRemap;

		bool isMouseTrapped = false; // HACK can be removed once we update SDL
	};

};
