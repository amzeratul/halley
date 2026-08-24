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
	setupMapping();
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
		const auto scanCode = static_cast<KeyCode>(event.scancode);
		const auto virtualCode = getHalleyKeyCodeFromSDLVirtualKeyCode(event.key);
		const auto mods = getMods(event.mod);
		switch (event.type) {
            case SDL_EVENT_KEY_DOWN:
				onKeyPressed(scanCode, virtualCode, mods);
				break;
			case SDL_EVENT_KEY_UP:
				onKeyReleased(scanCode, virtualCode, mods);
				break;
		}
	}
}

void InputKeyboardSDL3::setupMapping()
{
	auto& v = virtualKeyCodeToHalley;

	for (int16_t code = SDLK_A; code <= SDLK_Z; ++code) {
		v[code] = static_cast<KeyCode>(static_cast<int>(KeyCode::A) + (code - SDLK_A));
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
	v[SDLK_GRAVE] = KeyCode::Grave;
	v[SDLK_DELETE] = KeyCode::Delete;
};

KeyCode InputKeyboardSDL3::getHalleyKeyCodeFromSDLVirtualKeyCode(int sdlKeyCode) const
{
	if (sdlKeyCode & SDLK_SCANCODE_MASK)
	{
		return static_cast<KeyCode>(sdlKeyCode ^ SDLK_SCANCODE_MASK);
	}
	return virtualKeyCodeToHalley.value_or(sdlKeyCode, KeyCode::Unknown);
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
	const auto keycode = SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(code), SDL_KMOD_NONE, false);
	auto name = String(SDL_GetKeyName(keycode));

	if (name.startsWith("Keypad")) {
		name = name.replaceOne("Keypad", "KP");
	} else if (name == "Left Ctrl") {
		name = "LCtrl";
	} else if (name == "Right Ctrl") {
		name = "RCtrl";
	} else if (name == "Left Alt") {
		name = "LAlt";
	} else if (name == "Right Alt") {
		name = "RAlt";
	} else if (name == "PrintScreen") {
		name = "PrtSc";
	} else if (name == "ScrollLock") {
		name = "ScrLk";
	} else if (name == "PageDown") {
		name = "PgDn";
	} else if (name == "PageUp") {
		name = "PgUp";
	} else if (name == "Delete") {
		name = "Del";
	} else if (name == "Insert") {
		name = "Ins";
	} else if (name == "Numlock") {
		name = "Num";
	}

	return name;
}

void InputKeyboardSDL3::update()
{
	clearPresses();
}

void InputKeyboardSDL3::removeCapture(ITextInputCapture* capture)
{
	InputKeyboard::removeCapture(capture);

	Concurrent::execute(Executors::getMainRenderThread(), [=] {
		SDL_StopTextInput(system.getWindow(0)->getSDLWindow());
	});
}

std::unique_ptr<ITextInputCapture> InputKeyboardSDL3::makeTextInputCapture()
{
	Concurrent::execute(Executors::getMainRenderThread(), [=] {
		SDL_StartTextInput(system.getWindow(0)->getSDLWindow());
	});

	return InputKeyboard::makeTextInputCapture();
}
