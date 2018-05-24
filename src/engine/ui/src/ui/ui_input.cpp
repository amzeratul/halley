#include "ui_input.h"
#include "halley/utils/utils.h"

using namespace Halley;

UIInputResults::UIInputResults()
{
	reset();
}

UIInputResults::UIInputResults(std::array<bool, UIInput::NumberOfButtons> buttons, std::array<float, UIInput::NumberOfAxes> axes, std::array<int8_t, UIInput::NumberOfAxes> axesRepeat)
	: buttons(buttons)
	, axes(axes)
	, axesRepeat(axesRepeat)
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
	for (auto& a: axesRepeat) {
		a = 0;
	}
}

bool UIInputResults::isButtonPressed(UIInput::Button button) const
{
	return buttons[int(button)];
}

float UIInputResults::getAxis(UIInput::Axis axis) const
{
	return axes[int(axis)];
}

int UIInputResults::getAxisRepeat(UIInput::Axis axis) const
{
	return axesRepeat[int(axis)];
}

void UIInputResults::setButtonPressed(UIInput::Button button, bool pressed)
{
	buttons[int(button)] = pressed;
}

void UIInputResults::setAxis(UIInput::Axis axis, float value)
{
	axes[int(axis)] = value;
}

void UIInputResults::setAxisRepeat(UIInput::Axis axis, int value)
{
	axesRepeat[int(axis)] = clamp(value, -1, 1);
}
