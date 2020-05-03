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

void InputKeyboardSDL::processEvent(const SDL_Event& rawEvent)
{
	if (rawEvent.type == SDL_TEXTINPUT) {
		const SDL_TextInputEvent& event = rawEvent.text;
		onTextEntered(event.text);
	} else if (rawEvent.type == SDL_TEXTEDITING) {
		//const SDL_TextEditingEvent& event = rawEvent.edit;
	} else {
		const SDL_KeyboardEvent& event = rawEvent.key;
		switch (event.type) {
			case SDL_KEYDOWN:
				onKeyPressed(getKeyCode(event.keysym.sym), getMods(event.keysym.mod));
				break;
			case SDL_KEYUP:
				onKeyReleased(getKeyCode(event.keysym.sym), getMods(event.keysym.mod));
				break;
		}
	}
}

KeyCode InputKeyboardSDL::getKeyCode(int sdlKeyCode) const
{
	// Halley uses uppercase characters
	if (sdlKeyCode >= 'a' && sdlKeyCode <= 'z') {
		return KeyCode(sdlKeyCode - 32);
	}

	// Halley moves scancodes to +128 offset
	if ((sdlKeyCode & SDLK_SCANCODE_MASK) != 0) {
		return KeyCode((sdlKeyCode & ~SDLK_SCANCODE_MASK) + 128);
	}

	// Otherwise, should be compatible
	return KeyCode(sdlKeyCode);
}

KeyMods InputKeyboardSDL::getMods(int sdlMods) const
{
	int mods = 0;
	if ((sdlMods & KMOD_SHIFT) != 0) {
		mods |= static_cast<int>(KeyMods::Shift);
	}
	if ((sdlMods & KMOD_CTRL) != 0) {
		mods |= static_cast<int>(KeyMods::Ctrl);
	}
	if ((sdlMods & KMOD_ALT) != 0) {
		mods |= static_cast<int>(KeyMods::Alt);
	}
	if ((sdlMods & KMOD_GUI) != 0) {
		mods |= static_cast<int>(KeyMods::Mod);
	}
	return KeyMods(mods);
}

String InputKeyboardSDL::getButtonName(int code)
{
	switch (code) {
	case static_cast<int>(KeyCode::Esc):
		return "Esc";
	case static_cast<int>(KeyCode::Delete):
		return "Del";
	default:
		if (code >= static_cast<int>(KeyCode::A) && code <= static_cast<int>(KeyCode::Z)) {
			return String(static_cast<wchar_t>(code - static_cast<int>(KeyCode::A) + 'A'));
		} else {
			return SDL_GetKeyName(SDL_Keycode(code));
		}
	}
}

void InputKeyboardSDL::update()
{
	clearPresses();
}
