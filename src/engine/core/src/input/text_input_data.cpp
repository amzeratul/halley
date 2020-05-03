#include "halley/core/input/text_input_data.h"

#include "api/clipboard.h"
#include "halley/text/halleystring.h"
#include "halley/utils/utils.h"
#include "input/input_keyboard.h"

using namespace Halley;

TextInputData::TextInputData()
{
}

TextInputData::TextInputData(const String& str)
{
	setText(str);
}

TextInputData::TextInputData(StringUTF32 str)
{
	setText(std::move(str));
}

const StringUTF32& TextInputData::getText() const
{
	return text;
}

void TextInputData::setText(const String& text)
{
	setText(text.getUTF32());
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

void TextInputData::setLengthLimits(int min, std::optional<int> max)
{
	minLength = min;
	maxLength = max;
}

int TextInputData::getMinLength() const
{
	return minLength;
}

std::optional<int> TextInputData::getMaxLength() const
{
	return maxLength;
}

void TextInputData::insertText(const String& text)
{
	insertText(text.getUTF32());
}

void TextInputData::insertText(const StringUTF32& t)
{
	if (!t.empty() && !readOnly) {
		size_t insertSize = t.length();
		if (maxLength) {
			const int curSelLen = selection.end - selection.start;
			const size_t finalLen = text.size() - size_t(curSelLen) + insertSize;
			if (int(finalLen) > maxLength.value()) {
				insertSize = size_t(std::max(int64_t(0), int64_t(insertSize) + int64_t(maxLength.value()) - int64_t(finalLen)));
			}
		}

		const auto newEnd = int(selection.start + insertSize);
		setText(text.substr(0, selection.start) + t.substr(0, insertSize) + text.substr(selection.end));
		setSelection(newEnd);
	}
}

bool TextInputData::onKeyPress(KeyboardKeyPress c, IClipboard* clipboard)
{	
	if (c.is(KeyCode::Delete, KeyMods::None)) {
		onDelete();
		return true;
	}

	if (c.is(KeyCode::Backspace, KeyMods::None)) {
		onBackspace();
		return true;
	}
	
	if (c.is(KeyCode::Home, KeyMods::None) || c.is(KeyCode::PageUp, KeyMods::None)) {
		setSelection(0);
		return true;
	}
	
	if (c.is(KeyCode::End, KeyMods::None) || c.is(KeyCode::PageDown, KeyMods::None)) {
		setSelection(static_cast<int>(text.size()));
		return true;
	}
	
	if (c.is(KeyCode::Left, KeyMods::None)) {
		setSelection(getSelection().start - 1);
		return true;
	}
	
	if (c.is(KeyCode::Right, KeyMods::None)) {
		setSelection(getSelection().start + 1);
		return true;
	}
	
	if (c.is(KeyCode::C, KeyMods::Ctrl)) {
		if (clipboard) {
			clipboard->setData(String(text));
		}
		return true;
	}
	
	if (c.is(KeyCode::V, KeyMods::Ctrl)) {
		if (clipboard && !readOnly) {
			auto str = clipboard->getStringData();
			if (str) {
				insertText(str.value());
			}
		}
		return true;
	}
	
	if (c.is(KeyCode::X, KeyMods::Ctrl)) {
		if (clipboard && !readOnly) {
			clipboard->setData(String(text));
			setText(StringUTF32());
		}
		return true;
	}
	
	if (c.isPrintable()) {
		// Handled by text capture
		return true;
	}

	return false;
}

int TextInputData::getTextRevision() const
{
	return textRevision;
}

Range<int> TextInputData::getTotalRange() const
{
	return Range<int>(0, int(text.size()));
}

void TextInputData::setReadOnly(bool enable)
{
	readOnly = enable;
}

bool TextInputData::isReadOnly() const
{
	return readOnly;
}

void TextInputData::onDelete()
{
	if (readOnly) {
		return;
	}

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
	if (readOnly) {
		return;
	}

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
	++textRevision;
}
