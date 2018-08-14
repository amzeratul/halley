#include "ui_input.h"
#include "halley/utils/utils.h"

using namespace Halley;

UIInputResults::UIInputResults()
{
	reset();
}

void UIInputResults::reset()
{
	for (auto& b: buttonsPressed) {
		b = false;
	}
	for (auto& b: buttonsReleased) {
		b = false;
	}
	for (auto& b: buttonsHeld) {
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
	return buttonsPressed[int(button)];
}

bool UIInputResults::isButtonReleased(UIInput::Button button) const
{
	return buttonsReleased[int(button)];
}

bool UIInputResults::isButtonHeld(UIInput::Button button) const
{
	return buttonsHeld[int(button)];
}

float UIInputResults::getAxis(UIInput::Axis axis) const
{
	return axes[int(axis)];
}

int UIInputResults::getAxisRepeat(UIInput::Axis axis) const
{
	return axesRepeat[int(axis)];
}

void UIInputResults::setButton(UIInput::Button button, bool pressed, bool released, bool held)
{
	buttonsPressed[int(button)] = pressed;
	buttonsReleased[int(button)] = released;
	buttonsHeld[int(button)] = held;
}

void UIInputResults::setAxis(UIInput::Axis axis, float value)
{
	axes[int(axis)] = value;
}

void UIInputResults::setAxisRepeat(UIInput::Axis axis, int value)
{
	axesRepeat[int(axis)] = clamp(value, -1, 1);
}
