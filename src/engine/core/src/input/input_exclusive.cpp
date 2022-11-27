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




InputExclusive::InputExclusive(std::shared_ptr<InputVirtual> input, Vector<int> axes, Vector<int> buttons)
	: input(std::move(input))
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

		if (enabled) {
			buttonsExclusive.reserve(buttons.size());
			for (auto button: buttons) {
				buttonsExclusive.push_back(input->makeExclusiveButton(button, InputPriority::Maximum, ""));
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
	return enabled ? buttonsExclusive[code]->isPressed() : false;
}

bool InputExclusive::isButtonPressedRepeat(InputButton code)
{
	return enabled ? buttonsExclusive[code]->isPressedRepeat() : false;
}

bool InputExclusive::isButtonReleased(InputButton code)
{
	return enabled ? buttonsExclusive[code]->isReleased() : false;
}

bool InputExclusive::isButtonDown(InputButton code)
{
	return enabled ? buttonsExclusive[code]->isDown() : false;
}

float InputExclusive::getAxis(int i)
{
	return enabled ? input->getAxis(axes.at(i)) : 0.0f;
}

int InputExclusive::getAxisRepeat(int i)
{
	return enabled ? input->getAxisRepeat(axes.at(i)) : 0;
}
