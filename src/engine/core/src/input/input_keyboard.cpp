#include <halley/core/input/input_keyboard.h>
using namespace Halley;

TextInputData::TextInputData()
{
}

TextInputData::TextInputData(const String& str)
	: text(str.getUTF32())
{
}

TextInputData::TextInputData(StringUTF32 str)
	: text(std::move(str))
{
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
		if (selection.s > textSize) {
			selection.s = textSize;
		}
		if (selection.e > textSize) {
			selection.e = textSize;
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
	selection.s = selection.e = sel;
}

void TextInputData::setSelection(Range<int> sel)
{
	selection = sel;
	const int textSize = int(text.size());
	sel.s = clamp(sel.s, 0, textSize);
	sel.e = clamp(sel.e, 0, textSize);
	if (sel.s > sel.e) {
		std::swap(sel.s, sel.e);
	}
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
		const auto newEnd = int(selection.s + t.length());
		setText(text.substr(0, selection.s) + t + text.substr(selection.e));
		setSelection(newEnd);
	}
}

void TextInputData::onDelete()
{
	if (selection.s == selection.e) {
		setText(text.substr(0, selection.s) + text.substr(selection.s + 1));
	} else {
		setText(text.substr(0, selection.s) + text.substr(selection.e));
	}
}

void TextInputData::onBackspace()
{
	if (selection.s == selection.e) {
		if (selection.s > 0) { // If selection.s == 0, -1 causes it to overflow (unsigned). Shouldn't do anything in that case.
			setText(text.substr(0, selection.s - 1) + text.substr(selection.s));
			setSelection(selection.s - 1);
		}
	} else {
		setText(text.substr(0, selection.s) + text.substr(selection.e));
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
	capture->close();
}

bool TextInputCapture::update() const
{
	if (capture->isOpen()) {
		capture->update(inputData);
	}
	return capture->isOpen();
}

TextInputCapture InputKeyboard::captureText(TextInputData& textInputData, SoftwareKeyboardData data)
{
	return TextInputCapture(textInputData, std::move(data), makeTextInputCapture());
}
