#include <halley/core/input/input_keyboard.h>
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

void TextInputData::onDelete()
{
	if (selection.start == selection.end) {
		setText(text.substr(0, selection.start) + text.substr(selection.start + 1));
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
	: inputData(inputData)
	, capture(std::move(_capture))
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
		capture->update(inputData);
	}
	return capture->isOpen();
}

/*
	for (int letter = keyboard->getNextLetter(); letter != 0; letter = keyboard->getNextLetter()) {
		if (!ctrlDown && letter >= 32) {
			if (!maxLength || int(text.size()) < maxLength.get()) {
				text.insert(text.begin() + caret, letter);
				caret++;
				modified = true;
			}
		}
	}
 */

InputKeyboard::InputKeyboard(int nButtons)
	: InputButtonBase(nButtons)
{
}

TextInputCapture InputKeyboard::captureText(TextInputData& textInputData, SoftwareKeyboardData data)
{
	return TextInputCapture(textInputData, std::move(data), makeTextInputCapture());
}
