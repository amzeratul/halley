#pragma once

#include "input_button_base.h"
#include "text_input_capture.h"
#include "halley/data_structures/maybe.h"
#include "halley/maths/range.h"

namespace Halley {
	enum class TextControlCharacter;

	class InputKeyboard : public InputButtonBase {
	public:
		explicit InputKeyboard(int nButtons = -1);
		virtual ~InputKeyboard() = default;

		virtual TextInputCapture captureText(TextInputData& textInputData, SoftwareKeyboardData softKeyboardData);
		
		void onButtonPressed(int scanCode) override;
		void onButtonReleased(int scanCode) override;
		virtual void onTextControlCharacterGenerated(TextControlCharacter c);

	protected:
		virtual std::unique_ptr<ITextInputCapture> makeTextInputCapture() = 0;
	};
}
