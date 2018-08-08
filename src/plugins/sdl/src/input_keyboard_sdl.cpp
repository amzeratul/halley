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
using namespace Halley;

InputKeyboardSDL::InputKeyboardSDL()
	: InputKeyboard(SDL_NUM_SCANCODES)
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
				if (scancode == Keys::Backspace) {
					onTextEntered("\b");
				}
				if (scancode == Keys::Enter || scancode == Keys::KP_Enter) {
					onTextEntered("\n");
				}
				if (scancode == Keys::Tab) {
					onTextEntered("\t");
				}
				break;
			}
		}
	}
}


void InputKeyboardSDL::onTextEntered(const char* text)
{
	auto str = String(text).getUTF32();
	for (auto& c: captures) {
		c->onTextEntered(str);
	}
}

SDLTextInputCapture::SDLTextInputCapture(InputKeyboardSDL& parent)
	: parent(parent)
{
}

SDLTextInputCapture::~SDLTextInputCapture()
{
	parent.removeCapture(this);
}

void SDLTextInputCapture::open(const TextInputData& input, SoftwareKeyboardData softKeyboardData)
{
	currentlyOpen = true;
	buffer.clear();
}

void SDLTextInputCapture::close()
{
	currentlyOpen = false;
	buffer.clear();
}

bool SDLTextInputCapture::isOpen() const
{
	return currentlyOpen;
}

void SDLTextInputCapture::update(TextInputData& input)
{
	if (isOpen() && !buffer.empty()) {
		input.insertText(buffer);
		buffer.clear();
	}
}

void SDLTextInputCapture::onTextEntered(const StringUTF32& text)
{
	buffer += text;
}

std::unique_ptr<ITextInputCapture> InputKeyboardSDL::makeTextInputCapture()
{
	auto ptr = std::make_unique<SDLTextInputCapture>(*this);
	captures.insert(ptr.get());
	return ptr;
}

String InputKeyboardSDL::getButtonName(int code)
{
	switch (code) {
	case Keys::Esc:
		return "Esc";
	case Keys::Delete:
		return "Del";
	default:
		if (code >= Keys::A && code <= Keys::Z) {
			return String(wchar_t(code - Keys::A + 'A'));
		} else {
			return SDL_GetKeyName(SDL_Keycode(code));
		}
	}
}

void InputKeyboardSDL::removeCapture(SDLTextInputCapture* capture)
{
	captures.erase(capture);
}
