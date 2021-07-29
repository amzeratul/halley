#include <utility>
#include "widgets/ui_textinput.h"
#include "ui_style.h"
#include "ui_validator.h"
#include "halley/text/i18n.h"
#include "halley/ui/ui_data_bind.h"
#include "halley/support/logger.h"
#include "halley/core/input/input_keys.h"
#include "halley/core/api/system_api.h"

using namespace Halley;

UITextInput::UITextInput(String id, UIStyle style, String text, LocalisedString ghostText, std::shared_ptr<UIValidator> validator)
	: UIWidget(std::move(id), Vector2f(style.getFloat("minSize"), style.getFloat("minSize")), UISizer(UISizerType::Vertical), style.getBorder("innerBorder"))
	, style(style)
	, sprite(style.getSprite("box"))
	, caret(style.getSprite("caret"))
	, label(style.getTextRenderer("label"))
	, ghostLabel(style.getTextRenderer("labelGhost"))
	, text(text.getUTF32())
	, ghostText(std::move(ghostText))
{
	styleName = style.getName();
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
	return text.getSelection();
}

void UITextInput::setSelection(int selection)
{
	text.setSelection(selection);
}

void UITextInput::setSelection(Range<int> selection)
{
	text.setSelection(selection);
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

void UITextInput::draw(UIPainter& painter) const
{
	if (sprite.hasMaterial()) {
		painter.draw(sprite);
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
	int pos = clamp(text.getSelection().end, 0, int(text.getText().size()));
	if (pos != caretPos) {
		caretTime = 0;
		caretShowing = true;
		caretPos = pos;
		caretPhysicalPos = label.getCharacterPosition(caretPos, text.getText()).x;
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

void UITextInput::update(Time t, bool moved)
{
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

	// Update text labels
	const bool showGhost = text.getText().empty() && !isFocused();
	const bool showAutoComplete = autoCompleteCurOption.has_value();
	ghostText.checkForUpdates();
	ghostLabel.setText(showAutoComplete ? getAutoCompleteCaption() : (showGhost ? ghostText.getString().getUTF32() : StringUTF32()));
	label.setText(text.getText());

	// Position the text
	const float length = label.getExtents().x;
	const float capacityX = getSize().x - getInnerBorder().x - getInnerBorder().z;
	const float capacityY = getSize().y - getInnerBorder().y - getInnerBorder().w;
	const Vector2f startPos = getPosition() + Vector2f(getInnerBorder().x, getInnerBorder().y);
	if (length > capacityX) {
		textScrollPos.x = clamp(textScrollPos.x, std::max(0.0f, caretPhysicalPos - capacityX), std::min(length - capacityX, caretPhysicalPos));
		const auto clip = Rect4f(textScrollPos, textScrollPos + Vector2f(capacityX, capacityY));
		label.setClip(clip);
		ghostLabel.setClip(clip);
	} else {
		textScrollPos.x = 0;
		label.setClip();
		ghostLabel.setClip();
	}
	label.setPosition(startPos - textScrollPos);
	ghostLabel.setPosition(startPos - textScrollPos);

	// Position the caret
	caret.setPos(startPos - textScrollPos + Vector2f(caretPhysicalPos, 0));

	if (moved) {
		sprite.setPos(getPosition()).scaleTo(getSize());
	}

	if (text.isPendingSubmit()) {
		submit();
	}
}

void UITextInput::onFocus()
{
	caretTime = 0;
	caretShowing = true;
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
	}

	if (key.is(KeyCode::Enter) || key.is(KeyCode::KeypadEnter)) {
		if (!isMultiLine) {
			submit();
		}
		return true;
	}

	return false;
}

void UITextInput::pressMouse(Vector2f mousePos, int button)
{
	if (button == 0) {
		Vector2f labelClickPos = mousePos - label.getPosition();
		text.setSelection(int(label.getCharacterAt(labelClickPos)));
		updateCaret();
	}
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
