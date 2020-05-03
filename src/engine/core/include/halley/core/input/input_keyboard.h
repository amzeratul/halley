#pragma once

#include <set>

#include "input_button_base.h"
#include "input_keys.h"
#include "text_input_capture.h"

namespace Halley {
	enum class TextControlCharacter;

	struct KeyboardKeyPress {
		Keys key;
		KeyMods mod;

		KeyboardKeyPress() = default;
		KeyboardKeyPress(Keys key, KeyMods mod);

		bool operator ==(const KeyboardKeyPress& other) const;
		bool is(Keys key, KeyMods mod) const;
	};

	class InputKeyboard : public InputButtonBase {
	public:
		explicit InputKeyboard(int nButtons = -1, std::shared_ptr<IClipboard> clipboard = {});
		virtual ~InputKeyboard() = default;

		void onButtonPressed(int scanCode) override;
		void onButtonReleased(int scanCode) override;

		virtual TextInputCapture captureText(TextInputData& textInputData, SoftwareKeyboardData softKeyboardData);
		void removeCapture(ITextInputCapture* capture);

	protected:
		virtual std::unique_ptr<ITextInputCapture> makeTextInputCapture();

		void onTextEntered(const char* text);
		bool onKeyPress(KeyboardKeyPress c);

	private:
		std::set<ITextInputCapture*> captures;
		std::shared_ptr<IClipboard> clipboard;
	};
}
