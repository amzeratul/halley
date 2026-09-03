#pragma once

#include "halley/api/halley_api_internal.h"
#include <SDL3/SDL.h>
#include "input_joystick_sdl3.h"

namespace Halley {

	class InputGameControllerSDL3;
	class InputKeyboardSDL3;
	class InputMouseSDL3;
	class SystemSDL3;

	class InputSDL3 final : public InputAPIInternal {
		friend class HalleyAPI;

	public:
		explicit InputSDL3(SystemAPI& system);
		~InputSDL3() override;

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
		void processGameControllerDeviceEvent(const SDL_GamepadDeviceEvent& event);
		void processTouch(int type, long long touchId, long long fingerId, float x, float y);

		void addJoystick(SDL_JoystickID instanceId);
		std::shared_ptr<InputGameControllerSDL3> openGameController(SDL_JoystickID instanceId);

		SystemSDL3& system;
		
		Vector<std::shared_ptr<InputKeyboardSDL3>> keyboards;
		Vector<std::shared_ptr<InputJoystick>> joysticks;
		Vector<std::shared_ptr<InputMouseSDL3>> mice;

		HashMap<SDL_JoystickID, std::shared_ptr<InputJoystickSDL3>> sdlJoys;
		HashMap<SDL_JoystickID, std::shared_ptr<InputGameControllerSDL3>> sdlGameControllers;
		HashMap<int, std::shared_ptr<InputTouch>> touchEvents;
		HashMap<SDL_SystemCursor, SDL_Cursor*> cursors;
		std::optional<MouseCursorMode> curCursor;

		std::function<Vector2f(Vector2i)> mouseRemap;

		bool isMouseTrapped = false; // HACK can be removed once we update SDL
	};

}
