/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#include "input/input_button_base.h"
#include "halley/text/string_converter.h"

using namespace Halley;

InputButtonBase::InputButtonBase(int nButtons)
{
	if (nButtons != -1) init(nButtons);
}

void InputButtonBase::init(int nButtons)
{
	buttonPressed.resize(nButtons);
	buttonPressedRepeat.resize(nButtons);
	buttonReleased.resize(nButtons);
	buttonDown.resize(nButtons);
}

void InputButtonBase::onButtonPressed(int code)
{
	buttonPressedRepeat[code] = true;
	if (!buttonDown[code]) {
		buttonPressed[code] = true;
		// Note that this doesn't set released as false. It's possible to get a press and a release on the same step.
		buttonDown[code] = true;
	}
}

void InputButtonBase::onButtonReleased(int code)
{
	if (buttonDown[code]) {
		// See comment on method above
		buttonReleased[code] = true;
		buttonDown[code] = false;
	}
}

void InputButtonBase::onButtonStatus(int code, bool down)
{
	// This method should probably not be used with the two above
	// This is designed for polled input, such as XInput controllers
	bool wasDown = buttonDown[code] != 0;
	buttonDown[code] = down;
	if (wasDown && !down) buttonReleased[code] = true;
	if (!wasDown && down) {
		buttonPressed[code] = true;
		buttonPressedRepeat[code] = true;
	}
}

void InputButtonBase::setParent(InputDevice* p)
{
	parent = p;
}

InputDevice* InputButtonBase::getParent() const
{
	return parent;
}

void InputButtonBase::clearPresses()
{
	size_t len = buttonPressed.size();
	for (size_t i=0; i<len; i++) {
		buttonPressed[i] = 0;
		buttonPressedRepeat[i] = 0;
		buttonReleased[i] = 0;
	}
}

bool Halley::InputButtonBase::isAnyButtonPressed()
{
	for (size_t i=0; i<buttonPressed.size(); i++) {
		if (buttonPressed[i] != 0) {
			return true;
		}
	}
	return false;
}

bool InputButtonBase::isAnyButtonReleased()
{
	for (size_t i=0; i<buttonReleased.size(); i++) {
		if (buttonReleased[i] != 0) {
			return true;
		}
	}
	return false;
}

bool InputButtonBase::isButtonPressed(int code)
{
	if (code < 0 || code >= int(buttonPressed.size())) return false;
	return buttonPressed[code] != 0;
}

bool InputButtonBase::isButtonPressedRepeat(int code)
{
	if (code < 0 || code >= int(buttonPressedRepeat.size())) return false;
	return buttonPressedRepeat[code] != 0;
}

bool InputButtonBase::isButtonReleased(int code)
{
	if (code < 0 || code >= int(buttonReleased.size())) return false;
	return buttonReleased[code] != 0;
}

bool InputButtonBase::isButtonDown(int code)
{
	if (code < 0 || code >= int(buttonDown.size())) return false;
	return buttonDown[code] != 0;
}

Halley::String Halley::InputButtonBase::getButtonName(int code)
{
	return "Button" + toString(code);
}

void Halley::InputButtonBase::clearButton(int code)
{
	if (code >= 0 && code < int(buttonPressedRepeat.size())) {
		buttonPressed[code] = 0;
		buttonPressedRepeat[code] = 0;
		buttonDown[code] = 0;
		buttonReleased[code] = 0;
	}
}
