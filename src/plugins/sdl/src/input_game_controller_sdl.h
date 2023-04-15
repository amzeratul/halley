#pragma once

#include <SDL_events.h>
#include <SDL_gamecontroller.h>

#include "halley/input/input_joystick.h"

namespace Halley {

	class InputGameControllerSDL final : public InputJoystick {
		friend class InputSDL;
	public:
		InputGameControllerSDL(int number);
		~InputGameControllerSDL();

		void update(Time t) override;
		void close();

		std::string_view getName() const final override;
		String getMapping() const;
		JoystickType getJoystickType() const override { return JoystickType::Generic; }
		int getSDLJoystickId() const;

		int getButtonAtPosition(JoystickButtonPosition position) const override;

	private:
		SDL_GameController* controller = nullptr;
		int id;
		int idx;
		String name;

		void processEvent(const SDL_Event& event);
		void doSetVibration(float low, float high) override;
	};


}
