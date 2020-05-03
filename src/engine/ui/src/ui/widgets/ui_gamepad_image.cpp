#include "halley/ui/widgets/ui_gamepad_image.h"

using namespace Halley;

UIGamepadImage::UIGamepadImage(UIStyle style, JoystickButtonPosition button, std::function<Sprite(JoystickButtonPosition, JoystickType)> iconRetriever, Colour4f col)
	: UIImage(Sprite())
	, style(style)
	, button(button)
	, iconRetriever(std::move(iconRetriever))
	, colour(col)
{
	setOnlyEnabledWithInputs({ UIInputType::Gamepad });
}

void UIGamepadImage::update(Time t, bool moved)
{
	UIImage::update(t, moved);
}

void UIGamepadImage::setJoystickType(JoystickType type)
{
	if (type != curType) {
		curType = type;

		setSprite(iconRetriever(button, type).setColour(colour));
	}
}

void UIGamepadImage::onGamepadInput(const UIInputResults& input, Time time)
{
	if (input.isButtonPressed(UIGamepadInput::Button::Accept)) {
		sendEvent(UIEvent(UIEventType::ButtonClicked, getId()));
		playSound(style.getString("downSound"));
	}
}
