#pragma once

#include <set>
#include "halley/core/input/input_keyboard.h"
#include "halley/core/api/clipboard.h"
struct SDL_KeyboardEvent;

namespace Halley {

	class InputKeyboardSDL;

	class SDLTextInputCapture : public ITextInputCapture {
	public:
		SDLTextInputCapture(InputKeyboardSDL& parent);
		~SDLTextInputCapture();

		void open(TextInputData& input, SoftwareKeyboardData softKeyboardData) override;
		void close() override;
		bool isOpen() const override;
		void update() override;
		
		void onTextEntered(const StringUTF32& text);
		void onControlCharacter(TextControlCharacter c, std::shared_ptr<IClipboard> clipboard);

	private:
		bool currentlyOpen = false;
		InputKeyboardSDL& parent;
		TextInputData* textInput;
	};

	class InputKeyboardSDL : public InputKeyboard {
	public:
		std::unique_ptr<ITextInputCapture> makeTextInputCapture() override;
		String getButtonName(int code) override;

		void removeCapture(SDLTextInputCapture* capture);
		void update();

	private:
		InputKeyboardSDL(std::shared_ptr<IClipboard> clipboard);

		void processEvent(const SDL_Event &event);
		void onTextEntered(const char* text);
		void onTextControlCharacterGenerated(TextControlCharacter c) override;

		std::set<SDLTextInputCapture*> captures;
		std::shared_ptr<IClipboard> clipboard;

		friend class InputSDL;
	};
	
}
