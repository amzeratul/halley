#include "halley/input/text_input_data.h"

#include "halley/api/clipboard.h"
#include "halley/text/halleystring.h"
#include "halley/utils/utils.h"
#include "halley/input/input_keyboard.h"

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

void TextInputData::setTextFromSoftKeyboard(const String& text, bool accept)
{
	setText(text.getUTF32());
	if (accept) {
		pendingSubmit = true;
	}
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

TextInputData::Selection TextInputData::getSelection() const
{
	return selection;
}

void TextInputData::setSelection(int sel)
{
	setSelection(Selection(sel, sel));
}

void TextInputData::setSelection(Selection sel)
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
	const auto ctrlHeld = (c.mod & KeyMods::Ctrl) != KeyMods::None;
	const bool shiftHeld = (c.mod & KeyMods::Shift) != KeyMods::None;

	if (c.key == KeyCode::Delete) {
		onDelete(ctrlHeld);
		return true;
	}
	
	if (c.key == KeyCode::Backspace) {
		onBackspace(ctrlHeld);
		return true;
	}
	
	if (c.key == KeyCode::Home) {
		changeSelection(-1, shiftHeld, ctrlHeld ? ChangeSelectionMode::Document : ChangeSelectionMode::Line);
		return true;
	}
	
	if (c.key == KeyCode::End) {
		changeSelection(1, shiftHeld, ctrlHeld ? ChangeSelectionMode::Document : ChangeSelectionMode::Line);
		return true;
	}

	if (c.key == KeyCode::PageUp) {
		changeSelection(-1, shiftHeld, ChangeSelectionMode::Page);
		return true;
	}
	
	if (c.key == KeyCode::PageDown) {
		changeSelection(1, shiftHeld, ChangeSelectionMode::Page);
		return true;
	}

	if (!text.empty()) {
		if (c.key == KeyCode::Left) {
			changeSelection(-1, shiftHeld, ctrlHeld ? ChangeSelectionMode::Word : ChangeSelectionMode::Character);
			return true;
		}
		
		if (c.key == KeyCode::Right) {
			changeSelection(1, shiftHeld, ctrlHeld ? ChangeSelectionMode::Word : ChangeSelectionMode::Character);
			return true;
		}
	}
	
	if (c.is(KeyCode::C, KeyMods::Ctrl)) {
		if (clipboard) {
			const auto sel = getSelection();
			if (sel.start != sel.end) {
				clipboard->setData(String(text.substr(sel.start, sel.end - sel.start)));
			} else {
				clipboard->setData(String(text));
			}
		}
		return true;
	}
	
	if (c.is(KeyCode::V, KeyMods::Ctrl)) {
		if (clipboard && !readOnly) {
			if (auto str = clipboard->getStringData()) {
				insertText(str.value());
			}
		}
		return true;
	}
	
	if (c.is(KeyCode::X, KeyMods::Ctrl)) {
		if (clipboard && !readOnly) {
			const auto sel = getSelection();
			if (sel.start != sel.end) {
				clipboard->setData(String(text.substr(sel.start, sel.end - sel.start)));
				onDelete();
			} else {
				clipboard->setData(String(text));
				setText(StringUTF32());
			}
		}
		return true;
	}
	
	if (c.is(KeyCode::A, KeyMods::Ctrl)) {
		setSelection(Selection(0, static_cast<int>(text.length())));
	}

	if (c.is(KeyCode::Enter)) {
		if (captureSubmit) {
			pendingSubmit = true;
			return true;
		} else if (multiline) {
			insertText("\n");
			return true;
		}
	}

	if (multiline && c.is(KeyCode::Enter, KeyMods::Shift)) {
		insertText("\n");
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

bool TextInputData::isPendingSubmit()
{
	const auto value = pendingSubmit;
	pendingSubmit = false;
	return value;
}

void TextInputData::setCaptureSubmit(bool enable)
{
	captureSubmit = enable;
}

bool TextInputData::isMultiline() const
{
	return multiline;
}

void TextInputData::setMultiline(bool enable)
{
	multiline = enable;
}

void TextInputData::onTextModified()
{
	++textRevision;
}


void TextInputData::onDelete(bool wholeWord)
{
	if (readOnly) {
		return;
	}

	if (selection.start == selection.end) {
		if (selection.start < int(text.size())) {
			const auto endPos = wholeWord ? getWordBoundary(selection.start, 1) : selection.start + 1;
			setText(text.substr(0, selection.start) + text.substr(endPos));
		}
	} else {
		setText(text.substr(0, selection.start) + text.substr(selection.end));
		setSelection(selection.start);
	}
}

void TextInputData::onBackspace(bool wholeWord)
{
	if (readOnly) {
		return;
	}

	if (selection.start == selection.end) {
		if (selection.start > 0) { // If selection.s == 0, -1 causes it to overflow (unsigned). Shouldn't do anything in that case.
			const auto startPos = wholeWord ? getWordBoundary(selection.start, -1) : selection.start - 1;
			setText(text.substr(0, startPos) + text.substr(selection.start));
			setSelection(startPos);
		}
	} else {
		setText(text.substr(0, selection.start) + text.substr(selection.end));
		setSelection(selection.start);
	}
}

int TextInputData::getWordBoundary(int cursorPos, int dir) const
{
	auto classifyCharacter = [] (uint32_t c) -> int
	{
		if (c == '\n') {
			return 0;
		} else if (c == ' ' || c == '\t') {
			return 1;
		} else if (String::isAlphanumeric(c) || c == '_') {
			return 2;
		} else {
			return 3;
		}
	};

	const int firstPos = std::max(cursorPos + (dir == -1 ? -1 : 0), 0);
	const int len = static_cast<int>(text.length());
	auto category = classifyCharacter(text[firstPos]);

	bool first = true;
	for (int i = firstPos + dir; i >= 0 && i < len; i += dir) {
		const auto cat = classifyCharacter(text[i]);
		if (cat != category || category == 0) { // Category zero doesn't stack
			if (first && category == 1) {
				// Space before group, change category and keep going
				category = cat;
			} else {
				return dir == -1 ? i + 1 : i;
			}
		}
		first = false;
	}

	// Reached the end
	return dir == -1 ? 0 : len;
}

int TextInputData::getLineBoundary(int cursorPos, int dir) const
{
	// TODO
	return getTextBoundary(dir);
}

int TextInputData::getPageBoundary(int cursorPos, int dir) const
{
	// TODO
	return getTextBoundary(dir);
}

int TextInputData::getTextBoundary(int dir) const
{
	return dir == -1 ? 0 : static_cast<int>(text.size());
}

void TextInputData::changeSelection(int dir, bool shiftHeld, ChangeSelectionMode mode)
{
	const auto sel = getSelection();

	if (sel.end != sel.start && !shiftHeld) {
		setSelection(dir == 1 ? sel.end : sel.start);
		return;
	}

	int caret = shiftHeld ? sel.getCaret() : (dir == 1 ? sel.end : sel.start);

	switch (mode) {
	case ChangeSelectionMode::Character:
		caret = caret + dir;
		break;
	case ChangeSelectionMode::Word:
		caret = getWordBoundary(caret, dir);
		break;
	case ChangeSelectionMode::Line:
		caret = getLineBoundary(caret, dir);
		break;
	case ChangeSelectionMode::Page:
		caret = getPageBoundary(caret, dir);
		break;
	case ChangeSelectionMode::Document:
		caret = getTextBoundary(dir);
		break;
	}

	if (shiftHeld) {
		setSelection(Selection::fromAnchorAndCaret(sel.getAnchor(), caret));
	} else {
		setSelection(caret);
	}
}

void TextInputData::moveCursor(int position, KeyMods mods)
{
	if ((mods & KeyMods::Shift) != KeyMods::None) {
		const auto sel = getSelection();
		setSelection(Selection::fromAnchorAndCaret(sel.getAnchor(), position));
	} else {
		setSelection(position);
	}
}
