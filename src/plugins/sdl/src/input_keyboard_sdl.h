#pragma once

#include <set>
#include "halley/core/input/input_keyboard.h"
struct SDL_KeyboardEvent;

namespace Halley {

	class InputKeyboardSDL;

	class SDLTextInputCapture : public ITextInputCapture {
	public:
		SDLTextInputCapture(InputKeyboardSDL& parent);
		~SDLTextInputCapture();

		void open(const TextInputData& input, SoftwareKeyboardData softKeyboardData) override;
		void close() override;
		bool isOpen() const override;
		void update(TextInputData& input) override;
		
		void onTextEntered(const StringUTF32& text);

	private:
		bool currentlyOpen = false;
		InputKeyboardSDL& parent;
		StringUTF32 buffer;
	};

	class InputKeyboardSDL : public InputKeyboard {
	public:
		std::unique_ptr<ITextInputCapture> makeTextInputCapture() override;
		String getButtonName(int code) override;

		void removeCapture(SDLTextInputCapture* capture);

	private:
		InputKeyboardSDL();

		void processEvent(const SDL_Event &event);
		void onTextEntered(const char* text);

		std::set<SDLTextInputCapture*> captures;

		friend class InputSDL;
	};
	
}
