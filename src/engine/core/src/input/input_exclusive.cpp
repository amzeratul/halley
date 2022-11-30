#include "input/input_exclusive.h"
#include "input/input_virtual.h"

using namespace Halley;

InputExclusiveButton::InputExclusiveButton(InputVirtual& parent, InputPriority priority, InputButton button, String label)
	: parent(&parent)
	, button(button)
	, priority(priority)
	, label(std::move(label))
{
}

InputExclusiveButton::~InputExclusiveButton()
{
	if (parent) {
		parent->removeExclusiveButton(*this);
	}
}

void InputExclusiveButton::update(Time t)
{
}

bool InputExclusiveButton::isPressed() const
{
	return parent ? parent->isButtonPressed(button, activeBinds) : false;
}

bool InputExclusiveButton::isPressedRepeat() const
{
	return parent ? parent->isButtonPressedRepeat(button, activeBinds) : false;
}

bool InputExclusiveButton::isReleased() const
{
	return parent ? parent->isButtonReleased(button, activeBinds) : false;
}

bool InputExclusiveButton::isDown() const
{
	return parent ? parent->isButtonDown(button, activeBinds) : false;
}

const String& InputExclusiveButton::getLabel() const
{
	return label;
}

InputPriority InputExclusiveButton::getPriority() const
{
	return priority;
}

Vector<uint32_t>& InputExclusiveButton::getActiveBinds()
{
	return activeBinds;
}



InputExclusiveAxis::InputExclusiveAxis(InputVirtual& parent, InputPriority priority, int axis, String label)
	: parent(&parent)
	, axis(axis)
	, priority(priority)
	, label(std::move(label))
{
}

InputExclusiveAxis::~InputExclusiveAxis()
{
	if (parent) {
		parent->removeExclusiveAxis(*this);
	}
}

void InputExclusiveAxis::update(Time t)
{
	repeatValue = repeater.update(getAxis(), t);
}

float InputExclusiveAxis::getAxis() const
{
	return parent ? parent->getAxis(axis, activeBinds) : 0;
}

int InputExclusiveAxis::getAxisRepeat() const
{
	return repeatValue;
}

const String& InputExclusiveAxis::getLabel() const
{
	return label;
}

InputPriority InputExclusiveAxis::getPriority() const
{
	return priority;
}

Vector<uint32_t>& InputExclusiveAxis::getActiveBinds()
{
	return activeBinds;
}



InputExclusive::InputExclusive(std::shared_ptr<InputVirtual> input, InputPriority priority, Vector<int> axes, Vector<int> buttons)
	: input(std::move(input))
	, priority(priority)
	, axes(std::move(axes))
	, buttons(std::move(buttons))
{
	setEnabled(true);
}

void InputExclusive::setEnabled(bool enabled)
{
	if (enabled != this->enabled) {
		this->enabled = enabled;

		buttonsExclusive.clear();
		axesExclusive.clear();

		if (enabled) {
			buttonsExclusive.reserve(buttons.size());
			for (auto button: buttons) {
				buttonsExclusive.push_back(input->makeExclusiveButton(button, priority, ""));
			}

			axesExclusive.reserve(axes.size());
			for (auto axis: axes) {
				axesExclusive.push_back(input->makeExclusiveAxis(axis, priority, ""));
			}
		}
	}
}

bool InputExclusive::isEnabled() const
{
	return enabled;
}

InputType InputExclusive::getInputType() const
{
	return InputType::Virtual;
}

size_t InputExclusive::getNumberButtons()
{
	return buttons.size();
}

size_t InputExclusive::getNumberAxes()
{
	return axes.size();
}

String InputExclusive::getButtonName(int code) const
{
	return input->getButtonName(buttons.at(code));
}

bool InputExclusive::isButtonPressed(InputButton code)
{
	return enabled ? buttonsExclusive.at(code)->isPressed() : false;
}

bool InputExclusive::isButtonPressedRepeat(InputButton code)
{
	return enabled ? buttonsExclusive.at(code)->isPressedRepeat() : false;
}

bool InputExclusive::isButtonReleased(InputButton code)
{
	return enabled ? buttonsExclusive.at(code)->isReleased() : false;
}

bool InputExclusive::isButtonDown(InputButton code)
{
	return enabled ? buttonsExclusive.at(code)->isDown() : false;
}

float InputExclusive::getAxis(int i)
{
	return enabled ? axesExclusive.at(i)->getAxis() : 0.0f;
}

int InputExclusive::getAxisRepeat(int i)
{
	return enabled ? axesExclusive.at(i)->getAxisRepeat() : 0;
}
