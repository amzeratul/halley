#include <utility>
#include "halley/ui/widgets/ui_textinput.h"
#include "halley/ui/ui_style.h"
#include "halley/ui/ui_validator.h"
#include "halley/text/i18n.h"
#include "halley/ui/ui_data_bind.h"
#include "halley/support/logger.h"
#include "halley/input/input_keys.h"
#include "halley/api/system_api.h"

using namespace Halley;

namespace {
	Vector2f getInputSize(const UIStyle& style)
	{
		if (style.hasVector2f("minSize")) {
			return style.getVector2f("minSize");
		} else if (style.hasFloat("minSize")) {
			const auto s = style.getFloat("minSize");
			return Vector2f(s, s);
		} else {
			return Vector2f();
		}
	}
}

UITextInput::UITextInput(String id, UIStyle style, String text, LocalisedString ghostText, std::shared_ptr<UIValidator> validator)
	: UIWidget(std::move(id), getInputSize(style), UISizer(UISizerType::Vertical), style.getBorder("innerBorder"))
	, sprite(style.getSprite("box"))
	, caret(style.getSprite("caret"))
	, selectionBg(style.getSprite("selectionBg"))
	, label(style.getTextRenderer("label"))
	, ghostLabel(style.getTextRenderer("labelGhost"))
	, text(text.getUTF32())
	, ghostText(std::move(ghostText))
{
	styles.emplace_back(std::move(style));
	label.setText(text);
	setValidator(std::move(validator));
}

bool UITextInput::canInteractWithMouse() const
{
	return true;
}

UITextInput& UITextInput::setText(const String& t)
{
	setText(t.getUTF32());
	return *this;
}

UITextInput& UITextInput::setText(StringUTF32 t)
{
	if (t != text.getText()) {
		text.setText(std::move(t));
		onMaybeTextModified();
	}
	return *this;
}

String UITextInput::getText() const
{
	return String(text.getText());
}

UITextInput& UITextInput::setGhostText(LocalisedString t)
{
	ghostText = std::move(t);
	return *this;
}

LocalisedString UITextInput::getGhostText() const
{
	return ghostText;
}

void UITextInput::setShowGhostWhenFocused(bool show)
{
	showGhostWhenFocused = show;
}

TextRenderer& UITextInput::getTextLabel()
{
	return label;
}

std::optional<int> UITextInput::getMaxLength() const
{
	return text.getMaxLength();
}

void UITextInput::setMaxLength(std::optional<int> length)
{
	text.setLengthLimits(0, length);
}

Range<int> UITextInput::getSelection() const
{
	return text.getSelection().toRange();
}

void UITextInput::setSelection(int selection)
{
	text.setSelection(selection);
}

void UITextInput::setSelection(Range<int> selection)
{
	text.setSelection(TextInputData::Selection(selection));
}

void UITextInput::selectLast()
{
	text.setSelection(static_cast<int>(text.getText().size()));
}

void UITextInput::onManualControlActivate()
{
	focus();
}

void UITextInput::setAutoCompleteHandle(AutoCompleteHandle handle)
{
	autoCompleteHandle = std::move(handle);
	updateAutoCompleteOnTextModified();
}

bool UITextInput::canReceiveFocus() const
{
	return true;
}

void UITextInput::setReadOnly(bool enabled)
{
	text.setReadOnly(enabled);
}

bool UITextInput::isReadOnly() const
{
	return text.isReadOnly();
}

void UITextInput::setHistoryEnabled(bool enabled)
{
	historyEnabled = enabled;
}

bool UITextInput::isHistoryEnabled() const
{
	return historyEnabled;
}

void UITextInput::setMultiLine(bool enabled)
{
	multiLine = enabled;
}

bool UITextInput::isMultiLine() const
{
	return multiLine;
}

void UITextInput::setSelectAllOnClick(bool enabled)
{
	selectAllOnClick = enabled;
}

bool UITextInput::isSelectAllOnClick() const
{
	return selectAllOnClick;
}

void UITextInput::draw(UIPainter& painter) const
{
	if (sprite.hasMaterial()) {
		painter.draw(sprite);
	}

	if (isFocused()) {
		const auto sel = getSelection();
		if (sel.start != sel.end) {
			drawSelection(sel, painter);
		}
	}
	
	if (icon.hasMaterial()) {
		painter.draw(icon);
	}

	if (!ghostLabel.empty()) {
		painter.draw(ghostLabel);
	}
	
	if (!label.empty()) {
		painter.draw(label);
	}

	if (caretShowing && caret.hasMaterial()) {
		painter.draw(caret);
	}
}

void UITextInput::updateCaret()
{
	int pos = clamp(text.getSelection().getCaret(), 0, int(text.getText().size()));
	if (pos != caretPos) {
		caretTime = 0;
		caretShowing = true;
		caretPos = pos;
		caretPhysicalPos = label.getCharacterPosition(caretPos, label.getTextUTF32());
	}
}

void UITextInput::onMaybeTextModified()
{
	if (lastText != text.getText()) {
		lastText = text.getText();
		onTextModified();
	}
}

void UITextInput::onTextModified()
{
	if (getValidator()) {
		text.setText(getValidator()->onTextChanged(text.getText()));
	}
	updateCaret();

	const auto str = String(text.getText());
	sendEvent(UIEvent(UIEventType::TextChanged, getId(), str));
	notifyDataBind(str);

	updateAutoCompleteOnTextModified();
	updateHistoryOnTextModified();
}

void UITextInput::validateText()
{
	// TODO
	/*
	size_t removePos;
	while ((removePos = text.find('\r')) != StringUTF32::npos) {
		text = text.erase(removePos);
	}

	for (auto& c: text) {
		if (c == '\n' && !isMultiLine) {
			c = ' ';
		}
	}

	if (getValidator()) {
		text = getValidator()->onTextChanged(text);
	}
	*/
}

void UITextInput::onValidatorSet()
{
	if (getValidator()) {
		text.setText(getValidator()->onTextChanged(text.getText()));
	}
	updateCaret();
}

void UITextInput::submit()
{
	sendEvent(UIEvent(UIEventType::TextSubmit, getId(), getText()));

	if (historyEnabled) {
		addToHistory(getText());
	}

	if (clearOnSubmit) {
		setText(StringUTF32());
	}
}

void UITextInput::setClearOnSubmit(bool enabled)
{
	clearOnSubmit = enabled;
}

bool UITextInput::isClearOnSubmit() const
{
	return clearOnSubmit;
}

void UITextInput::setIcon(Sprite icon, Vector4f border)
{
	this->icon = std::move(icon);
	iconBorder = border;
}

void UITextInput::setAutoSize(std::optional<Range<float>> range)
{
	autoSizeRange = range;
}

void UITextInput::update(Time t, bool moved)
{
	// Update auto text
	const bool showGhost = text.getText().empty() && (!isFocused() || showGhostWhenFocused || isReadOnly());
	const bool showAutoComplete = autoCompleteCurOption.has_value();
	ghostText.checkForUpdates();
	ghostLabel.setText(showAutoComplete ? getAutoCompleteCaption() : (showGhost ? ghostText.getString().getUTF32() : StringUTF32()));

	// Update text
	const auto textBounds = getTextBounds();
	if (multiLine) {
		const auto extents = label.getExtents(text.getText());
		if (extents.x > textBounds.getWidth()) {
			label.setText(label.split(text.getText(), textBounds.getWidth() - 1));
		} else {
			label.setText(text.getText());
		}
	} else {
		label.setText(text.getText());
	}

	// Caret
	if (isFocused()) {
		caretTime += float(t);
		if (caretTime > 0.4f) {
			caretTime -= 0.4f;
			caretShowing = !caretShowing;
		}

		if (t > 0.000001f) {
			// Update can be called more than once. Only one of them will have non-zero time.
			updateCaret();
			onMaybeTextModified();
		}
	} else {
		caretTime = 0;
		caretShowing = false;
	}

	// Size
	auto textSize = label.empty() ? ghostLabel.getExtents() : label.getExtents();
	if (autoSizeRange) {
		const auto border = getTextInnerBorder();
		setMinSize(Vector2f(std::clamp(textSize.x + border.x + border.z, autoSizeRange->start, autoSizeRange->end), getMinimumSize().y));
	}

	// Position the text
	if (!multiLine && textSize.x > textBounds.getWidth()) {
		textScrollPos.x = clamp(textScrollPos.x, std::max(0.0f, caretPhysicalPos.x - textBounds.getWidth()), std::min(textSize.x - textBounds.getWidth(), caretPhysicalPos.x));
		const auto clip = Rect4f(textScrollPos, textScrollPos + textBounds.getSize());
		label.setClip(clip);
		ghostLabel.setClip(clip);
	} else if (textSize.y > textBounds.getHeight()) {
		const float caretTop = caretPhysicalPos.y;
		const float caretBottom = caretPhysicalPos.y + label.getLineHeight();
		textScrollPos.y = clamp(textScrollPos.y, std::max(0.0f, caretBottom - textBounds.getHeight()), std::min(textSize.y - textBounds.getHeight(), caretTop));
		const auto clip = Rect4f(textScrollPos, textScrollPos + textBounds.getSize());
		label.setClip(clip);
		ghostLabel.setClip(clip);
	} else {
		textScrollPos.x = 0;
		label.setClip();
		ghostLabel.setClip();
	}
	const auto textPos = textBounds.getTopLeft() - textScrollPos;
	label.setPosition(textPos);
	ghostLabel.setPosition(textPos);

	// Position the caret
	caret.setPos(textPos + caretPhysicalPos);

	// Position the icon
	if (icon.hasMaterial()) {
		icon.setPos(getPosition() + getInnerBorder().xy() + iconBorder.xy());
	}

	if (moved) {
		sprite.setPos(getPosition()).scaleTo(getSize());
	}

	if (text.isPendingSubmit()) {
		submit();
	}
	
	if (clickTimeout > 0) {
		--clickTimeout;
	}
}

Vector4f UITextInput::getTextInnerBorder() const
{
	Vector4f border = getInnerBorder();
	if (icon.hasMaterial()) {
		border.x += icon.getSize().x + iconBorder.x + iconBorder.z;
	}
	return border;
}

Rect4f UITextInput::getTextBounds() const
{
	const auto border = getTextInnerBorder();
	const Vector2f startPos = getPosition() + Vector2f(border.x, border.y);
	const auto size = getSize() - border.xy() - border.zw();
	return Rect4f(startPos, size.x, size.y);
}

void UITextInput::onFocus(bool byClicking)
{
	caretTime = 0;
	caretShowing = true;
	if (!byClicking || selectAllOnClick) {
		setSelection(text.getTotalRange());
	}

	if (byClicking && selectAllOnClick) {
		clickTimeout = 2;
	}
}

TextInputData* UITextInput::getTextInputData()
{
	return &text;
}

bool UITextInput::onKeyPress(KeyboardKeyPress key)
{
	if (autoCompleteHandle) {
		if (key.is(KeyCode::Tab)) {
			autoComplete();
			return true;
		}
	}

	if (historyEnabled) {
		if (key.is(KeyCode::Up)) {
			navigateHistory(1);
			return true;
		}

		if (key.is(KeyCode::Down)) {
			navigateHistory(-1);
			return true;
		}
	} else if (multiLine) {
		if (key.key == KeyCode::Up) {
			const auto from = label.getCharacterPosition(text.getSelection().getCaret());
			text.moveCursor(static_cast<int>(label.getCharacterAt(from + Vector2f(0, -label.getLineHeight()))), key.mod);
			return true;
		}

		if (key.key == KeyCode::Down) {
			const auto from = label.getCharacterPosition(text.getSelection().getCaret());
			text.moveCursor(static_cast<int>(label.getCharacterAt(from + Vector2f(0, label.getLineHeight()))), key.mod);
			return true;
		}
	}

	if (key.is(KeyCode::Enter) || key.is(KeyCode::KeypadEnter)) {
		submit();
		return true;
	}

	return false;
}

void UITextInput::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	if (clickTimeout > 0) {
		return;
	}

	if (button == 0) {
		const auto labelClickPos = mousePos - label.getPosition();
		const auto pos = static_cast<int>(label.getCharacterAt(labelClickPos));

		if ((keyMods & KeyMods::Shift) != KeyMods::None) {
			const auto sel = text.getSelection();
			text.setSelection(TextInputData::Selection::fromAnchorAndCaret(sel.getAnchor(), pos));
		} else {
			text.setSelection(pos);
		}
		updateCaret();
		mouseHeld = true;
	}
}

void UITextInput::releaseMouse(Vector2f mousePos, int button)
{
	if (button == 0) {
		mouseHeld = false;
	}
}

void UITextInput::onMouseOver(Vector2f mousePos)
{
	if (mouseHeld) {
		const auto labelClickPos = mousePos - label.getPosition();
		const auto pos = static_cast<int>(label.getCharacterAt(labelClickPos));
		const auto sel = text.getSelection();
		text.setSelection(TextInputData::Selection::fromAnchorAndCaret(sel.getAnchor(), pos));
	}
}

bool UITextInput::isFocusLocked() const
{
	return mouseHeld;
}

void UITextInput::readFromDataBind()
{
	setText(getDataBind()->getStringData());
}

void UITextInput::autoComplete()
{
	if (autoCompleteCurOption) {
		if (text.getText() == autoCompleteOptions[autoCompleteCurOption.value()]) {
			// Cycle to next
			autoCompleteCurOption = modulo(static_cast<int>(autoCompleteCurOption.value()) + 1, static_cast<int>(autoCompleteOptions.size()));
		}
		
		modifiedByAutoComplete = true;
		setText(autoCompleteOptions[autoCompleteCurOption.value()]);
		selectLast();
	}
}

void UITextInput::updateAutoCompleteOnTextModified()
{
	if (!modifiedByAutoComplete) {
		userInputForAutoComplete = text.getText();
	}
	modifiedByAutoComplete = false;
	
	refreshAutoCompleteOptions();
}

void UITextInput::refreshAutoCompleteOptions()
{
	if (!autoCompleteHandle || userInputForAutoComplete.empty()) {
		autoCompleteCurOption.reset();
	} else {
		const StringUTF32 prevOption = autoCompleteCurOption ? autoCompleteOptions[autoCompleteCurOption.value()] : StringUTF32();
		
		autoCompleteOptions = autoCompleteHandle(userInputForAutoComplete);
		if (autoCompleteOptions.empty()) {
			autoCompleteCurOption.reset();
		} else {
			const auto iter = std::find(autoCompleteOptions.begin(), autoCompleteOptions.end(), prevOption);
			if (iter != autoCompleteOptions.end()) {
				autoCompleteCurOption = iter - autoCompleteOptions.begin();
			} else {
				autoCompleteCurOption = 0;
			}
		}
	}
}

StringUTF32 UITextInput::getAutoCompleteCaption() const
{
	StringUTF32 result = autoCompleteOptions[autoCompleteCurOption.value()];
	if (autoCompleteOptions.size() > 1) {
		const String append = " [" + toString(autoCompleteCurOption.value() + 1) + "/" + toString(autoCompleteOptions.size()) + "]";
		result += append.getUTF32();
	}
	return result;
}

void UITextInput::addToHistory(String str)
{
	history.emplace(history.begin(), std::move(str));
	if (history.size() > 20) {
		history.pop_back();
	}
	historyCurOption.reset();
}

void UITextInput::navigateHistory(int delta)
{
	if (history.empty()) {
		return;
	}
	
	const int historySize = static_cast<int>(history.size());
	const int startValue = static_cast<int>(historyCurOption.value_or(delta > 0 ? -1 : historySize));
	const int val = modulo(startValue + delta, historySize);
	historyCurOption = val;

	modifiedByHistory = true;
	setText(history[val]);
	selectLast();
}

void UITextInput::updateHistoryOnTextModified()
{
	if (!modifiedByHistory) {
		historyCurOption.reset();
	}
	modifiedByHistory = false;
}

void UITextInput::drawSelection(Range<int> sel, UIPainter& painter) const
{
	int lastStart = sel.start;
	for (int i = sel.start; i < sel.end; ++i) {
		if (label.getText()[i] == '\n') {
			drawSelectionRow(Range<int>(lastStart, i), painter);
			lastStart = ++i;
		}
	}

	drawSelectionRow(Range<int>(lastStart, sel.end), painter);
}

void UITextInput::drawSelectionRow(Range<int> row, UIPainter& painter) const
{
	if (row.end <= row.start) {
		return;
	}
	const auto left = label.getCharacterPosition(row.start);
	const auto right = label.getCharacterPosition(row.end);
	Sprite bg = selectionBg.clone()
		.setPosition(left + label.getPosition())
		.scaleTo(Vector2f(right.x - left.x, label.getLineHeight()));
	painter.draw(bg, true);
}
