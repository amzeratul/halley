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
		void open(SDL_JoystickID instanceId);
		void close();
		bool isOpen() const;

		std::string_view getName() const final;
		String getMapping() const;
		JoystickType getJoystickType() const override { return JoystickType::Generic; }
		SDL_JoystickID getSDLJoystickId() const;

		int getButtonAtPosition(JoystickButtonPosition position) const override;

#ifdef WITH_GDK
		String getButtonName(int code) const override;
#endif

	private:
		SDL_Gamepad* controller = nullptr;
		SDL_JoystickID id;
		String name;

		void processEvent(const SDL_Event& event);
		void doSetVibration(float low, float high) override;

		friend class InputSDL3;
	};

}
