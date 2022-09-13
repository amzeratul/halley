#include "input/input_exclusive.h"
#include "input/input_virtual.h"

using namespace Halley;

InputExclusiveButton::InputExclusiveButton(InputVirtual& parent, InputPriority priority, InputButton button)
	: parent(&parent)
	, button(button)
	, priority(priority)
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
	return active && parent ? parent->isButtonPressed(button) : false;
}

bool InputExclusiveButton::isPressedRepeat() const
{
	return active && parent ? parent->isButtonPressedRepeat(button) : false;
}

bool InputExclusiveButton::isReleased() const
{
	return active && parent ? parent->isButtonReleased(button) : false;
}

bool InputExclusiveButton::isDown() const
{
	return active && parent ? parent->isButtonDown(button) : false;
}
