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

InputKeyboardSDL::InputKeyboardSDL(std::shared_ptr<IClipboard> clipboard)
	: InputKeyboard(SDL_NUM_SCANCODES)
	, clipboard(clipboard)
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


void InputKeyboardSDL::onTextEntered(const char* text)
{
	auto str = String(text).getUTF32();
	for (auto& c: captures) {
		c->onTextEntered(str);
	}
}

void InputKeyboardSDL::onTextControlCharacterGenerated(TextControlCharacter chr)
{
	for (auto& c: captures) {
		c->onControlCharacter(chr, clipboard);
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

void SDLTextInputCapture::open(TextInputData& input, SoftwareKeyboardData softKeyboardData)
{
	currentlyOpen = true;
	textInput = &input;
}

void SDLTextInputCapture::close()
{
	currentlyOpen = false;
	textInput = nullptr;
}

bool SDLTextInputCapture::isOpen() const
{
	return currentlyOpen;
}

void SDLTextInputCapture::update()
{
}

void SDLTextInputCapture::onTextEntered(const StringUTF32& text)
{
	textInput->insertText(text);
}

void SDLTextInputCapture::onControlCharacter(TextControlCharacter c, std::shared_ptr<IClipboard> clipboard)
{
	textInput->onControlCharacter(c, clipboard);
}

std::unique_ptr<ITextInputCapture> InputKeyboardSDL::makeTextInputCapture()
{
	auto ptr = std::make_unique<SDLTextInputCapture>(*this);
	captures.insert(ptr.get());
	return std::move(ptr);
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

void InputKeyboardSDL::update()
{
	clearPresses();
}
