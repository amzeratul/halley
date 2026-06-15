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
#include "halley/input/input_keys.h"
#include "halley/input/text_input_data.h"
using namespace Halley;

InputKeyboardSDL::InputKeyboardSDL(std::shared_ptr<IClipboard> clipboard)
	: InputKeyboard(SDL_NUM_SCANCODES, clipboard)
{
	setupMapping();
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
		const auto scanCode = static_cast<KeyCode>(event.keysym.scancode);
		const auto virtualCode = getHalleyKeyCodeFromSDLVirtualKeyCode(event.keysym.sym);
		const auto mods = getMods(event.keysym.mod);

		switch (event.type) {
			case SDL_KEYDOWN:
				onKeyPressed(scanCode, virtualCode, mods);
				break;
			case SDL_KEYUP:
				onKeyReleased(scanCode, virtualCode, mods);
				break;
		}
	}
}

void InputKeyboardSDL::setupMapping()
{
	auto& v = virtualKeyCodeToHalley;

	for (int16_t code = SDLK_a; code <= SDLK_z; ++code) {
		v[code] = static_cast<KeyCode>(static_cast<int>(KeyCode::A) + (code - SDLK_a));
	}

	v[SDLK_RETURN] = KeyCode::Enter;
    v[SDLK_ESCAPE] = KeyCode::Esc;
    v[SDLK_BACKSPACE] = KeyCode::Backspace;
    v[SDLK_TAB] = KeyCode::Tab;
    v[SDLK_SPACE] = KeyCode::Space;
    v[SDLK_COMMA] = KeyCode::Comma;
    v[SDLK_MINUS] = KeyCode::Minus;
    v[SDLK_PERIOD] = KeyCode::Period;
    v[SDLK_SLASH] = KeyCode::Slash;
    v[SDLK_0] = KeyCode::Num0;
    v[SDLK_1] = KeyCode::Num1;
    v[SDLK_2] = KeyCode::Num2;
    v[SDLK_3] = KeyCode::Num3;
    v[SDLK_4] = KeyCode::Num4;
    v[SDLK_5] = KeyCode::Num5;
    v[SDLK_6] = KeyCode::Num6;
    v[SDLK_7] = KeyCode::Num7;
    v[SDLK_8] = KeyCode::Num8;
    v[SDLK_9] = KeyCode::Num9;
    v[SDLK_SEMICOLON] = KeyCode::Semicolon;
    v[SDLK_LEFTBRACKET] = KeyCode::LeftBracket;
    v[SDLK_BACKSLASH] = KeyCode::Backslash;
    v[SDLK_RIGHTBRACKET] = KeyCode::RightBracket;
    v[SDLK_BACKQUOTE] = KeyCode::Grave;
    v[SDLK_DELETE] = KeyCode::Delete;
};

KeyCode InputKeyboardSDL::getHalleyKeyCodeFromSDLVirtualKeyCode(int sdlKeyCode) const
{
	if (sdlKeyCode & SDLK_SCANCODE_MASK) {
		return static_cast<KeyCode>(sdlKeyCode ^ SDLK_SCANCODE_MASK);
	}

	return virtualKeyCodeToHalley.value_or(sdlKeyCode, KeyCode::Unknown);
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

String InputKeyboardSDL::getButtonName(int code) const
{
	auto keycode = SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(code));
	return String(SDL_GetKeyName(keycode));
}

void InputKeyboardSDL::update()
{
	clearPresses();
}
