#pragma once
#include "ui_image.h"
#include "halley/core/input/input_joystick.h"

namespace Halley {
	class UIGamepadImage : public UIImage {
	public:
		UIGamepadImage(JoystickButtonPosition button, std::function<Sprite(JoystickButtonPosition, JoystickType)> iconRetriever, Colour4f col = Colour4f(1, 1, 1, 1));
		void update(Time t, bool moved) override;

	protected:
		void setJoystickType(JoystickType type) override;
		void onInput(const UIInputResults& input) override;

	private:
		JoystickButtonPosition button;
		std::function<Sprite(JoystickButtonPosition, JoystickType)> iconRetriever;
		Colour4f colour;
		Maybe<JoystickType> curType;
	};
}
