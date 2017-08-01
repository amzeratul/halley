#pragma once
#include "ui_image.h"
#include "input/input_joystick.h"

namespace Halley {
	class UIGamepadImage : public UIImage {
	public:
		UIGamepadImage(JoystickButtonPosition button, std::function<Sprite(JoystickButtonPosition, JoystickType)> iconRetriever, Colour4f col = Colour4f(1, 1, 1, 1));
		void update(Time t, bool moved) override;

	protected:
		void updateInputDevice(InputDevice& device) override;

	private:
		JoystickButtonPosition button;
		std::function<Sprite(JoystickButtonPosition, JoystickType)> iconRetriever;
		Colour4f colour;
		Maybe<JoystickType> curType;

		void setCurrentJoystickType(JoystickType type);
	};
}
