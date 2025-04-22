#pragma once

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>
#include "halley/input/input_joystick.h"

namespace Halley {

	class InputGameControllerSDL3 final : public InputJoystick
	{
	public:
		explicit InputGameControllerSDL3(SDL_JoystickID instanceId);
		~InputGameControllerSDL3() override;

		void update(Time t) override;
		void close();

		std::string_view getName() const final;
		String getMapping() const;
		JoystickType getJoystickType() const override { return JoystickType::Generic; }
		SDL_JoystickID getSDLJoystickId() const;

		int getButtonAtPosition(JoystickButtonPosition position) const override;

	private:
		SDL_Gamepad* controller = nullptr;
		SDL_JoystickID id;
		String name;

		void processEvent(const SDL_Event& event);
		void doSetVibration(float low, float high) override;

		friend class InputSDL3;
	};

}
