#pragma once

#include <SDL3/SDL_events.h>
#include "halley/input/input_joystick.h"

namespace Halley {

	class InputJoystickSDL3 final : public InputJoystick
	{
	public:
		explicit InputJoystickSDL3(SDL_JoystickID instanceId);
		~InputJoystickSDL3() override;

		void update(Time t) override;
		void close();

		std::string_view getName() const final;
		JoystickType getJoystickType() const override { return JoystickType::Generic; }
		SDL_JoystickID getSDLJoystickId() const;

		int getButtonAtPosition(JoystickButtonPosition position) const override;

	private:
		SDL_Joystick* joystick = nullptr;
		SDL_JoystickID id = 0;
		int baseButtons = 0;
		String name;

		int getSDLAxisIndex(int axis);
		void processAxisEvent(int axis, float value);
		void processEvent(const SDL_Event& event);

		void doSetVibration(float low, float high) override;

		friend class InputSDL3;
	};

	class InputJoystickHatSDL3 : public InputButtonBase {
	private:
		InputJoystickHatSDL3() : InputButtonBase(4) {}

		void processEvent(const SDL_Event& event);
	
		friend class InputJoystickSDL3;
	};

}
