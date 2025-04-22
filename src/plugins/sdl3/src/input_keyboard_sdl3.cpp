#include <SDL3/SDL.h>
#include "input_keyboard_sdl3.h"

#include "sdl3_window.h"
#include "halley/input/input_keys.h"
#include "halley/input/text_input_data.h"
#include "system_sdl3.h"

using namespace Halley;

InputKeyboardSDL3::InputKeyboardSDL3(SystemAPI& system, std::shared_ptr<IClipboard> clipboard)
	: system(dynamic_cast<SystemSDL3&>(system))
	, InputKeyboard(SDL_SCANCODE_COUNT, clipboard)
{
}

void InputKeyboardSDL3::processEvent(const SDL_Event& rawEvent)
{
	if (rawEvent.type == SDL_EVENT_TEXT_INPUT) {
		const SDL_TextInputEvent& event = rawEvent.text;
		onTextEntered(event.text);
	} else if (rawEvent.type == SDL_EVENT_TEXT_EDITING) {
		//const SDL_TextEditingEvent& event = rawEvent.edit;
	} else {
		const SDL_KeyboardEvent& event = rawEvent.key;
		switch (event.type) {
            case SDL_EVENT_KEY_DOWN:
				onKeyPressed(getKeyCode(event.key), getMods(event.mod));
				break;
			case SDL_EVENT_KEY_UP:
				onKeyReleased(getKeyCode(event.key), getMods(event.mod));
				break;
		}
	}
}

KeyCode InputKeyboardSDL3::getKeyCode(int sdlKeyCode) const
{
	// Halley uses uppercase characters
	if (sdlKeyCode >= 'a' && sdlKeyCode <= 'z') {
		return KeyCode(sdlKeyCode - 32);
	}

	// SDL has a special code for Delete for some reason
	if (sdlKeyCode == SDLK_DELETE) {
		return KeyCode::Delete;
	}

	// Halley moves scancodes to +128 offset
	if ((sdlKeyCode & SDLK_SCANCODE_MASK) != 0) {
		return KeyCode((sdlKeyCode & ~SDLK_SCANCODE_MASK) + 128);
	}

	// Otherwise, should be compatible
	return KeyCode(sdlKeyCode);
}

KeyMods InputKeyboardSDL3::getMods(int sdlMods) const
{
	int mods = 0;
	if ((sdlMods & SDL_KMOD_SHIFT) != 0) {
		mods |= static_cast<int>(KeyMods::Shift);
	}
	if ((sdlMods & SDL_KMOD_CTRL) != 0) {
		mods |= static_cast<int>(KeyMods::Ctrl);
	}
	if ((sdlMods & SDL_KMOD_ALT) != 0) {
		mods |= static_cast<int>(KeyMods::Alt);
	}
	if ((sdlMods & SDL_KMOD_GUI) != 0) {
		mods |= static_cast<int>(KeyMods::Mod);
	}
	return KeyMods(mods);
}

String InputKeyboardSDL3::getButtonName(int code) const
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
			auto *str = SDL_GetKeyName(SDL_SCANCODE_TO_KEYCODE(code - 128));
			return str;
		}
	}
}

void InputKeyboardSDL3::update()
{
	clearPresses();
}

void InputKeyboardSDL3::removeCapture(ITextInputCapture* capture)
{
	InputKeyboard::removeCapture(capture);
	SDL_StopTextInput(system.getWindow(0)->getSDLWindow());
}

std::unique_ptr<ITextInputCapture> InputKeyboardSDL3::makeTextInputCapture()
{
	SDL_StartTextInput(system.getWindow(0)->getSDLWindow());
	return InputKeyboard::makeTextInputCapture();
}
