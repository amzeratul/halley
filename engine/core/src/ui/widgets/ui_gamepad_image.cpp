#include "halley/core/ui/widgets/ui_gamepad_image.h"

using namespace Halley;

UIGamepadImage::UIGamepadImage(JoystickButtonPosition button, std::function<Sprite(JoystickButtonPosition, JoystickType)> iconRetriever, Colour4f col)
	: UIImage(Sprite())
	, button(button)
	, iconRetriever(iconRetriever)
	, colour(col)
{
	setOnlyEnabledWithInputs({ UIInputType::Gamepad });
}

void UIGamepadImage::update(Time t, bool moved)
{
	UIImage::update(t, moved);
}

void UIGamepadImage::updateInputDevice(InputDevice& device)
{
	auto* joy = dynamic_cast<InputJoystick*>(&device);
	if (joy) {
		setCurrentJoystickType(joy->getType());
	} else {
		auto* virt = dynamic_cast<InputVirtual*>(&device);
		if (virt) {
			auto joy2 = std::dynamic_pointer_cast<InputJoystick>(virt->getLastDevice());
			if (joy2) {
				setCurrentJoystickType(joy2->getType());
			}
		}
	}
}

void UIGamepadImage::setCurrentJoystickType(JoystickType type)
{
	if (type != curType) {
		curType = type;

		setSprite(iconRetriever(button, type).setColour(colour));
	}
}
