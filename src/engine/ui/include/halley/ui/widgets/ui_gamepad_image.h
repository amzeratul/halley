#pragma once
#include "ui_image.h"
#include "../ui_style.h"
#include "halley/input/input_joystick.h"

namespace Halley {
	class UIGamepadImage : public UIImage {
	public:
		UIGamepadImage(String id, JoystickButtonPosition button, std::function<Sprite(JoystickButtonPosition, JoystickType)> iconRetriever, Colour4f col = Colour4f(1, 1, 1, 1));
		void update(Time t, bool moved) override;

		void setJoystickType(JoystickType type) override;
		void setButtonPosition(JoystickButtonPosition position);

	private:
		JoystickButtonPosition button;
		std::function<Sprite(JoystickButtonPosition, JoystickType)> iconRetriever;
		Colour4f colour;
		std::optional<JoystickType> curType;

		void refreshSprite();
	};
}
