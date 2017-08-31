#include "ui/ui_input.h"
#include "halley/utils/utils.h"

using namespace Halley;

UIInputResults::UIInputResults()
{
	reset();
}

UIInputResults::UIInputResults(std::array<bool, UIInput::NumberOfButtons> buttons, std::array<int8_t, UIInput::NumberOfAxes> axes)
	: buttons(buttons)
	, axes(axes)
{
}

void UIInputResults::reset()
{
	for (auto& b: buttons) {
		b = false;
	}
	for (auto& a: axes) {
		a = 0;
	}
}

bool UIInputResults::isButtonPressed(UIInput::Button button) const
{
	return buttons[int(button)];
}

int UIInputResults::getAxisRepeat(UIInput::Axis axis) const
{
	return axes[int(axis)];
}

void UIInputResults::setButtonPressed(UIInput::Button button, bool pressed)
{
	buttons[int(button)] = pressed;
}

void UIInputResults::setAxisRepeat(UIInput::Axis axis, int value)
{
	axes[int(axis)] = clamp(value, -1, 1);
}
