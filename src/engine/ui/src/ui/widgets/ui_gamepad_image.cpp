#include "halley/ui/widgets/ui_gamepad_image.h"

using namespace Halley;

UIGamepadImage::UIGamepadImage(JoystickButtonPosition button, std::function<Sprite(JoystickButtonPosition, JoystickType)> iconRetriever, Colour4f col)
	: UIImage(Sprite())
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

void UIGamepadImage::onInput(const UIInputResults& input, Time time)
{
	if (input.isButtonPressed(UIInput::Button::Accept)) {
		sendEvent(UIEvent(UIEventType::ButtonClicked, getId()));
	}
}
