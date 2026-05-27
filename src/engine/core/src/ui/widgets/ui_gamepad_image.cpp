#include "halley/ui/widgets/ui_gamepad_image.h"

using namespace Halley;

UIGamepadImage::UIGamepadImage(String id, JoystickButtonPosition button, std::function<Sprite(JoystickButtonPosition, JoystickType)> iconRetriever, Colour4f col)
	: UIImage(std::move(id), Sprite())
	, button(button)
	, iconRetriever(std::move(iconRetriever))
	, colour(col)
{
	setAlwaysShow(false, true);
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

void UIGamepadImage::setAlwaysShow(bool enabled, bool force)
{
	if (enabled != alwaysShow || force) {
		alwaysShow = enabled;
		if (enabled) {
			setOnlyEnabledWithInputs({});
		} else {
			setOnlyEnabledWithInputs({ UIInputType::Gamepad });	
		}

		refreshSprite();
	}
}

void UIGamepadImage::refreshSprite()
{
	if (curType && curType != JoystickType::None) {
		setSprite(iconRetriever(button, curType.value()).setColour(colour));
	} else {
		setSprite(Sprite());
	}
}
