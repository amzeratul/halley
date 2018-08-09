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

UITextInput::UITextInput(std::shared_ptr<InputKeyboard> keyboard, String id, UIStyle style, String text, LocalisedString ghostText)
	: UIWidget(id, {}, UISizer(UISizerType::Vertical), style.getBorder("innerBorder"))
	, keyboard(std::move(keyboard))
	, style(style)
	, sprite(style.getSprite("box"))
	, caret(style.getSprite("caret"))
	, label(style.getTextRenderer("label"))
	, text(text.getUTF32())
	, ghostText(std::move(ghostText))
{
	label.setText(text);
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
		onTextModified();
	}
	return *this;
}

String UITextInput::getText() const
{
	return String(text.getText());
}

UITextInput& UITextInput::setGhostText(const LocalisedString& t)
{
	ghostText = t;
	return *this;
}

LocalisedString UITextInput::getGhostText() const
{
	return ghostText;
}

Maybe<int> UITextInput::getMaxLength() const
{
	return text.getMaxLength();
}

void UITextInput::setMaxLength(Maybe<int> length)
{
	text.setLengthLimits(0, length);
}

void UITextInput::onManualControlActivate()
{
	getRoot()->setFocus(shared_from_this());
}

void UITextInput::draw(UIPainter& painter) const
{
	painter.draw(sprite);
	painter.draw(label);

	if (caretShowing && caret.hasMaterial()) {
		painter.draw(caret);
	}
}

void UITextInput::updateTextInput()
{
	if (!keyboard) {
		return;
	}

	if (capture) {
		bool ok = capture->update();
		if (!ok) {
			capture = {};
		}
	}

	updateCaret();

	onTextModified();
	
	if (keyboard->isButtonPressed(Keys::Enter) || keyboard->isButtonPressed(Keys::KP_Enter)) {
		if (!isMultiLine) {
			sendEvent(UIEvent(UIEventType::TextSubmit, getId(), getText()));
		}
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

void UITextInput::onTextModified()
{
	if (getValidator()) {
		text.setText(getValidator()->onTextChanged(text.getText()));
	}
	updateCaret();

	const auto str = String(text.getText());
	sendEvent(UIEvent(UIEventType::TextChanged, getId(), str));
	notifyDataBind(str);
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
			updateTextInput();
		}
	} else {
		caretTime = 0;
		caretShowing = false;
	}

	// Update text label
	if (text.getText().empty() && !isFocused()) {
		ghostText.checkForUpdates();
		label = style.getTextRenderer("labelGhost");
		label.setText(ghostText);
	} else {
		label = style.getTextRenderer("label");
		label.setText(text.getText());
	}

	// Position the text
	const float length = label.getExtents().x;
	const float capacityX = getSize().x - getInnerBorder().x - getInnerBorder().z;
	const float capacityY = getSize().y - getInnerBorder().y - getInnerBorder().w;
	const Vector2f startPos = getPosition() + Vector2f(getInnerBorder().x, getInnerBorder().y);
	if (length > capacityX) {
		textScrollPos.x = clamp(textScrollPos.x, std::max(0.0f, caretPhysicalPos - capacityX), std::min(length - capacityX, caretPhysicalPos));
		label.setClip(Rect4f(textScrollPos, textScrollPos + Vector2f(capacityX, capacityY)));
	} else {
		textScrollPos.x = 0;
		label.setClip();
	}
	label.setPosition(startPos - textScrollPos);

	// Position the caret
	caret.setPos(startPos - textScrollPos + Vector2f(caretPhysicalPos, 0));

	if (moved) {
		sprite.setPos(getPosition()).scaleTo(getSize());
	}
}

void UITextInput::onFocus()
{
	caretTime = 0;
	caretShowing = true;

	capture = std::make_unique<TextInputCapture>(keyboard->captureText(text, {}));
}

void UITextInput::onFocusLost()
{
	capture.reset();
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
