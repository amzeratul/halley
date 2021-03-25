#pragma once

#include <set>

#include "input_button_base.h"
#include "input_keys.h"
#include "text_input_capture.h"

namespace Halley {
	enum class TextControlCharacter;

	struct KeyboardKeyPress {
		KeyCode key;
		KeyMods mod;

		KeyboardKeyPress() = default;
		KeyboardKeyPress(KeyCode key, KeyMods mod = KeyMods::None);

		bool operator ==(const KeyboardKeyPress& other) const;
		bool is(KeyCode key, KeyMods mod = KeyMods::None) const;
		bool isPrintable() const;
	};

	class InputKeyboard : public InputButtonBase {
	public:
		explicit InputKeyboard(int nButtons = -1, std::shared_ptr<IClipboard> clipboard = {});
		virtual ~InputKeyboard() = default;

		void onKeyPressed(KeyCode code, KeyMods mods);
		void onKeyReleased(KeyCode code, KeyMods mods);

		gsl::span<const KeyboardKeyPress> getPendingKeys() const;

		virtual TextInputCapture captureText(TextInputData& textInputData, SoftwareKeyboardData softKeyboardData);
		void removeCapture(ITextInputCapture* capture);

	protected:
		virtual std::unique_ptr<ITextInputCapture> makeTextInputCapture();

		void onTextEntered(const char* text);
		bool sendKeyPress(KeyboardKeyPress c);

		void onButtonsCleared() override;
		
	private:
		std::set<ITextInputCapture*> captures;
		std::shared_ptr<IClipboard> clipboard;
		std::vector<KeyboardKeyPress> keyPresses;
	};
}
