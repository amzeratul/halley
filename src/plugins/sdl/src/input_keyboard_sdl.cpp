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

#include <SDL.h>
#include "input_keyboard_sdl.h"
#include "halley/core/input/input_keys.h"
#include "halley/core/input/text_input_data.h"
using namespace Halley;

InputKeyboardSDL::InputKeyboardSDL(std::shared_ptr<IClipboard> clipboard)
	: InputKeyboard(SDL_NUM_SCANCODES, clipboard)
{
	SDL_StartTextInput();
}

void InputKeyboardSDL::processEvent(const SDL_Event &_event)
{
	if (_event.type == SDL_TEXTINPUT) {
		const SDL_TextInputEvent& event = _event.text;
		onTextEntered(event.text);
	} else if (_event.type == SDL_TEXTEDITING) {
		//const SDL_TextEditingEvent& event = _event.edit;
	} else {
		const SDL_KeyboardEvent& event = _event.key;
		switch (event.type) {
			case SDL_KEYUP:
				onButtonReleased(event.keysym.scancode);
				break;
			case SDL_KEYDOWN:
			{
				int scancode = event.keysym.scancode;
				onButtonPressed(scancode);
				break;
			}
		}
	}
}

String InputKeyboardSDL::getButtonName(int code)
{
	switch (code) {
	case static_cast<int>(Keys::Esc):
		return "Esc";
	case static_cast<int>(Keys::Delete):
		return "Del";
	default:
		if (code >= static_cast<int>(Keys::A) && code <= static_cast<int>(Keys::Z)) {
			return String(static_cast<wchar_t>(code - static_cast<int>(Keys::A) + 'A'));
		} else {
			return SDL_GetKeyName(SDL_Keycode(code));
		}
	}
}

void InputKeyboardSDL::update()
{
	clearPresses();
}
