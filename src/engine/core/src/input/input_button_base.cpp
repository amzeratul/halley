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

void InputButtonBase::onButtonsCleared()
{}

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
	const size_t len = buttonPressed.size();
	for (size_t i = 0; i<len; i++) {
		buttonPressed[i] = 0;
		buttonPressedRepeat[i] = 0;
		buttonReleased[i] = 0;
	}
	onButtonsCleared();
}

bool InputButtonBase::isAnyButtonPressed()
{
	return std::any_of(buttonPressed.begin(), buttonPressed.end(), [] (const auto& v) { return v != 0; });
}

bool InputButtonBase::isAnyButtonPressedRepeat()
{
	return std::any_of(buttonPressedRepeat.begin(), buttonPressedRepeat.end(), [] (const auto& v) { return v != 0; });
}

bool InputButtonBase::isAnyButtonReleased()
{
	return std::any_of(buttonReleased.begin(), buttonReleased.end(), [] (const auto& v) { return v != 0; });
}

bool InputButtonBase::isAnyButtonDown()
{
	return std::any_of(buttonDown.begin(), buttonDown.end(), [] (const auto& v) { return v != 0; });
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

bool InputButtonBase::isButtonPressed(KeyCode code)
{
	return isButtonPressed(static_cast<int>(code));
}

bool InputButtonBase::isButtonPressedRepeat(KeyCode code)
{
	return isButtonPressedRepeat(static_cast<int>(code));
}

bool InputButtonBase::isButtonReleased(KeyCode code)
{
	return isButtonReleased(static_cast<int>(code));
}

bool InputButtonBase::isButtonDown(KeyCode code)
{
	return isButtonDown(static_cast<int>(code));
}

String InputButtonBase::getButtonName(int code)
{
	return "Button" + toString(code);
}

void InputButtonBase::clearButton(int code)
{
	if (code >= 0 && code < int(buttonPressedRepeat.size())) {
		buttonPressed[code] = 0;
		buttonPressedRepeat[code] = 0;
		buttonDown[code] = 0;
		buttonReleased[code] = 0;
	}
}

void InputButtonBase::clearButtonPress(int code)
{
	if (code >= 0 && code < int(buttonPressedRepeat.size())) {
		buttonPressed[code] = 0;
		buttonPressedRepeat[code] = 0;
	}
}

void InputButtonBase::clearButtonRelease(int code)
{
	if (code >= 0 && code < int(buttonPressedRepeat.size())) {
		buttonReleased[code] = 0;
	}
}
