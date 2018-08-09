#include <halley/core/input/input_keyboard.h>
#include "input/input_keys.h"
#include "api/clipboard.h"
using namespace Halley;

TextInputData::TextInputData()
{
}

TextInputData::TextInputData(const String& str)
{
	setText(str.getUTF32());
}

TextInputData::TextInputData(StringUTF32 str)
{
	setText(std::move(str));
}

const StringUTF32& TextInputData::getText() const
{
	return text;
}

void TextInputData::setText(StringUTF32 _text)
{
	if (text != _text) {
		text = std::move(_text);
		const int textSize = int(text.size());
		if (selection.start > textSize) {
			selection.start = textSize;
		}
		if (selection.end > textSize) {
			selection.end = textSize;
		}
		onTextModified();
	}
}

Range<int> TextInputData::getSelection() const
{
	return selection;
}

void TextInputData::setSelection(int sel)
{
	setSelection(Range<int>(sel, sel));
}

void TextInputData::setSelection(Range<int> sel)
{
	const int textSize = int(text.size());
	sel.start = clamp(sel.start, 0, textSize);
	sel.end = clamp(sel.end, 0, textSize);
	if (sel.start > sel.end) {
		std::swap(sel.start, sel.end);
	}
	selection = sel;
}

void TextInputData::setLengthLimits(int min, Maybe<int> max)
{
	minLength = min;
	maxLength = max;
}

int TextInputData::getMinLength() const
{
	return minLength;
}

Maybe<int> TextInputData::getMaxLength() const
{
	return maxLength;
}

void TextInputData::insertText(const String& text)
{
	insertText(text.getUTF32());
}

void TextInputData::insertText(const StringUTF32& t)
{
	if (!t.empty()) {
		const auto newEnd = int(selection.start + t.length());
		setText(text.substr(0, selection.start) + t + text.substr(selection.end));
		setSelection(newEnd);
	}
}

void TextInputData::onControlCharacter(TextControlCharacter c, std::shared_ptr<IClipboard> clipboard)
{
	switch (c) {
	case TextControlCharacter::Delete:
		onDelete();
		break;
	case TextControlCharacter::Backspace:
		onBackspace();
		break;
	case TextControlCharacter::Home:
	case TextControlCharacter::PageUp:
		setSelection(0);
		break;
	case TextControlCharacter::End:
	case TextControlCharacter::PageDown:
		setSelection(int(text.size()));
		break;
	case TextControlCharacter::Left:
		setSelection(getSelection().start - 1);
		break;
	case TextControlCharacter::Right:
		setSelection(getSelection().start + 1);
		break;
	}

	if (clipboard) {
		switch (c) {
		case TextControlCharacter::Copy:
			clipboard->setData(String(text));
			break;
		case TextControlCharacter::Paste:
		{
			auto str = clipboard->getStringData();
			if (str) {
				insertText(str.get());
			}
			break;
		}
		case TextControlCharacter::Cut:
			clipboard->setData(String(text));
			setText(StringUTF32());
			break;
		}
	}
}

void TextInputData::onDelete()
{
	if (selection.start == selection.end) {
		if (selection.start < int(text.size())) {
			setText(text.substr(0, selection.start) + text.substr(selection.start + 1));
		}
	} else {
		setText(text.substr(0, selection.start) + text.substr(selection.end));
	}
}

void TextInputData::onBackspace()
{
	if (selection.start == selection.end) {
		if (selection.start > 0) { // If selection.s == 0, -1 causes it to overflow (unsigned). Shouldn't do anything in that case.
			const auto start = selection.start;
			setText(text.substr(0, start - 1) + text.substr(start));
			setSelection(start - 1);
		}
	} else {
		setText(text.substr(0, selection.start) + text.substr(selection.end));
	}
}

void TextInputData::onTextModified()
{
	// TODO
}

TextInputCapture::TextInputCapture(TextInputData& inputData, SoftwareKeyboardData softKeyboardData, std::unique_ptr<ITextInputCapture> _capture)
	: capture(std::move(_capture))
{
	capture->open(inputData, std::move(softKeyboardData));
}

TextInputCapture::~TextInputCapture()
{
	if (capture) {
		capture->close();
	}
}

bool TextInputCapture::update() const
{
	if (capture->isOpen()) {
		capture->update();
	}
	return capture->isOpen();
}

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

	Maybe<TextControlCharacter> code;

	if (!shiftDown && !ctrlDown) {
		switch (scanCode) {
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
		}
	}

	if (ctrlDown && !shiftDown) {
		switch (scanCode) {
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
		}
	}

	if (shiftDown && !ctrlDown) {
		switch (scanCode) {
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
		}
	}

	if (code) {
		onTextControlCharacterGenerated(code.get());
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
