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
