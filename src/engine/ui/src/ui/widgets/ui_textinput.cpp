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

void UITextInput::onManualControlActivate()
{
	focus();
}

void UITextInput::setAutoCompleteHandle(AutoCompleteHandle handle)
{
	autoCompleteHandle = std::move(handle);
	updateAutoComplete();
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

	updateAutoComplete();
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
	const bool showAutoComplete = !autoCompleteText.empty();
	ghostText.checkForUpdates();
	ghostLabel.setText(showAutoComplete ? autoCompleteText : (showGhost ? ghostText.getString().getUTF32() : StringUTF32()));
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
		sendEvent(UIEvent(UIEventType::TextSubmit, getId(), getText()));
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
			if (!autoCompleteText.empty()) {
				setText(autoCompleteText);
				setSelection(int(text.getText().size()));
			}
			return true;
		}
		
		if (key.is(KeyCode::Up)) {
			autoCompleteCurOption = autoCompleteOptions > 0 ? modulo(autoCompleteCurOption - 1, autoCompleteOptions) : 0;
			return true;
		}

		if (key.is(KeyCode::Down)) {
			autoCompleteCurOption = autoCompleteOptions > 0 ? modulo(autoCompleteCurOption + 1, autoCompleteOptions) : 0;
			return true;
		}
	}

	if (key.is(KeyCode::Enter) || key.is(KeyCode::KeypadEnter)) {
		if (!isMultiLine) {
			sendEvent(UIEvent(UIEventType::TextSubmit, getId(), getText()));
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

void UITextInput::updateAutoComplete()
{
	if (!autoCompleteHandle || text.getText().empty()) {
		autoCompleteOptions = 0;
		autoCompleteCurOption = 0;
		autoCompleteText.clear();
	} else {
		const auto result = autoCompleteHandle(text.getText());
		if (result.empty()) {
			autoCompleteOptions = 0;
			autoCompleteCurOption = 0;
			autoCompleteText.clear();
		} else {
			autoCompleteOptions = int(result.size());
			autoCompleteCurOption = modulo(autoCompleteCurOption, autoCompleteOptions);
			autoCompleteText = result[autoCompleteCurOption];
		}
	}
}
