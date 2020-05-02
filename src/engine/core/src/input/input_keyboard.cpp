#include <halley/core/input/input_keyboard.h>
#include "input/input_keys.h"
#include "input/text_input_capture.h"
#include "input/text_input_data.h"
using namespace Halley;

InputKeyboard::InputKeyboard(int nButtons)
	: InputButtonBase(nButtons)
{
}

TextInputCapture InputKeyboard::captureText(TextInputData& textInputData, SoftwareKeyboardData data)
{
	return TextInputCapture(textInputData, std::move(data), makeTextInputCapture());
}

void InputKeyboard::onButtonPressed(int scanCode)
{
	const bool shiftDown = isButtonDown(Keys::LShift) || isButtonDown(Keys::RShift);
	const bool ctrlDown = isButtonDown(Keys::LCtrl) || isButtonDown(Keys::RCtrl);

	std::optional<TextControlCharacter> code;

	if (!shiftDown && !ctrlDown) {
		switch (Keys(scanCode)) {
		case Keys::Backspace:
			code = TextControlCharacter::Backspace;
			break;
		case Keys::Delete:
			code = TextControlCharacter::Delete;
			break;
		case Keys::Enter:
		case Keys::KP_Enter:
			code = TextControlCharacter::Enter;
			break;
		case Keys::Tab:
			code = TextControlCharacter::Tab;
			break;
		case Keys::Left:
			code = TextControlCharacter::Left;
			break;
		case Keys::Right:
			code = TextControlCharacter::Right;
			break;
		case Keys::Up:
			code = TextControlCharacter::Up;
			break;
		case Keys::Down:
			code = TextControlCharacter::Down;
			break;
		case Keys::PageUp:
			code = TextControlCharacter::PageUp;
			break;
		case Keys::PageDown:
			code = TextControlCharacter::PageDown;
			break;
		case Keys::Home:
			code = TextControlCharacter::Home;
			break;
		case Keys::End:
			code = TextControlCharacter::End;
			break;
		default:
			break;
		}
	}

	if (ctrlDown && !shiftDown) {
		switch (Keys(scanCode)) {
		case Keys::C:
			code = TextControlCharacter::Copy;
			break;
		case Keys::V:
			code = TextControlCharacter::Paste;
			break;
		case Keys::X:
			code = TextControlCharacter::Cut;
			break;
		case Keys::Z:
			code = TextControlCharacter::Undo;
			break;
		case Keys::Y:
			code = TextControlCharacter::Redo;
			break;
		case Keys::A:
			code = TextControlCharacter::SelectAll;
			break;
		default:
			break;
		}
	}

	if (shiftDown && !ctrlDown) {
		switch (Keys(scanCode)) {
		case Keys::Left:
			code = TextControlCharacter::SelectLeft;
			break;
		case Keys::Right:
			code = TextControlCharacter::SelectRight;
			break;
		case Keys::Up:
			code = TextControlCharacter::SelectUp;
			break;
		case Keys::Down:
			code = TextControlCharacter::SelectDown;
			break;
		default:
			break;
		}
	}

	if (code) {
		onTextControlCharacterGenerated(code.value());
	}
	InputButtonBase::onButtonPressed(scanCode);
}

void InputKeyboard::onButtonReleased(int scanCode)
{
	InputButtonBase::onButtonReleased(scanCode);
}

void InputKeyboard::onTextControlCharacterGenerated(TextControlCharacter c)
{
}
