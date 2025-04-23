#include "halley/ui/ui_input.h"
#include "halley/utils/utils.h"

using namespace Halley;

UIInputResults::UIInputResults()
{
	reset();
}

void UIInputResults::reset()
{
	buttonsPressed.fill(false);
	buttonsPressedRepeat.fill(false);
	buttonsReleased.fill(false);
	buttonsHeld.fill(false);
	axes.fill(0);
	axesRepeat.fill(0);
}

bool UIInputResults::isButtonPressed(UIGamepadInput::Button button) const
{
	return buttonsPressed[int(button)];
}

bool UIInputResults::isButtonPressedRepeat(UIGamepadInput::Button button) const
{
	return buttonsPressedRepeat[int(button)];
}

bool UIInputResults::isButtonReleased(UIGamepadInput::Button button) const
{
	return buttonsReleased[int(button)];
}

bool UIInputResults::isButtonHeld(UIGamepadInput::Button button) const
{
	return buttonsHeld[int(button)];
}

float UIInputResults::getAxis(UIGamepadInput::Axis axis) const
{
	return axes[int(axis)];
}

int UIInputResults::getAxisRepeat(UIGamepadInput::Axis axis) const
{
	return axesRepeat[int(axis)];
}

void UIInputResults::setButton(UIGamepadInput::Button button, bool pressed, bool pressedRepeat, bool released, bool held)
{
	buttonsPressed[int(button)] = pressed;
	buttonsPressedRepeat[int(button)] = pressedRepeat;
	buttonsReleased[int(button)] = released;
	buttonsHeld[int(button)] = held;
}

void UIInputResults::setAxis(UIGamepadInput::Axis axis, float value)
{
	axes[int(axis)] = value;
}

void UIInputResults::setAxisRepeat(UIGamepadInput::Axis axis, int value)
{
	axesRepeat[int(axis)] = clamp(value, -1, 1);
}

void UIInputResults::clearPress(UIGamepadInput::Button button)
{
	bool isHeld = isButtonHeld(button);
	setButton(button, false, false, false, isHeld);
}
