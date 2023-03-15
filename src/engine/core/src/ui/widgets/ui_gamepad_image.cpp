#include "halley/ui/widgets/ui_gamepad_image.h"

using namespace Halley;

UIGamepadImage::UIGamepadImage(String id, JoystickButtonPosition button, std::function<Sprite(JoystickButtonPosition, JoystickType)> iconRetriever, Colour4f col)
	: UIImage(std::move(id), Sprite())
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

		refreshSprite();
	}
}

void UIGamepadImage::setButtonPosition(JoystickButtonPosition position)
{
	if (button != position) {
		button = position;

		refreshSprite();
	}
}

void UIGamepadImage::refreshSprite()
{
	if (curType) {
		setSprite(iconRetriever(button, curType.value()).setColour(colour));
	} else {
		setSprite(Sprite());
	}
}
