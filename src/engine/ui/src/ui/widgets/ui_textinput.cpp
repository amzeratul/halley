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

UITextInput::UITextInput(std::shared_ptr<InputDevice> keyboard, String id, UIStyle style, String text, LocalisedString ghostText)
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
	if (t != text) {
		text = std::move(t);
		setCaretPosition(int(text.size()));
		onTextModified();
	}
	return *this;
}

UITextInput& UITextInput::insertText(const String& t, size_t pos)
{
	if (!t.isEmpty()) {
		auto toInsert = t.getUTF32();
		text.insert(pos, toInsert);
		setCaretPosition(int(pos + toInsert.length()));
		onTextModified();
	}
	return *this;
}

String UITextInput::getText() const
{
	return String(text);
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
	return maxLength;
}

void UITextInput::setMaxLength(Maybe<int> length)
{
	maxLength = length;
}

void UITextInput::onManualControlActivate()
{
	if (keyboard->isSoftwareKeyboard()) {
		SoftwareKeyboardData data;
		data.initial = String(text);
		data.minLength = 0;
		data.maxLength = maxLength ? maxLength.get() : -1;
		setText(keyboard->getSoftwareKeyboardInput(data));
	} else {
		getRoot()->setFocus(shared_from_this());
	}
}

void UITextInput::setClipboard(std::shared_ptr<IClipboard> c)
{
	clipboard = std::move(c);
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

	const bool ctrlDown = keyboard->isButtonDown(Keys::LCtrl) || keyboard->isButtonDown(Keys::RCtrl);

	if (ctrlDown) {
		if (clipboard) {
			if (keyboard->isButtonPressed(Keys::C)) {
				clipboard->setData(String(text));
			}
			if (keyboard->isButtonPressed(Keys::X)) {
				clipboard->setData(String(text));
				setText("");
			}
			if (keyboard->isButtonPressed(Keys::V)) {
				auto str = clipboard->getStringData();
				if (str) {
					insertText(str.get(), caretPos);
				}
			}
		}
	}

	bool modified = false;
	int caret = caretPos;

	if (keyboard->isButtonPressedRepeat(Keys::Delete)) {
		if (caret < int(text.size())) {
			text.erase(text.begin() + caret);
			modified = true;
		}
	}
	
	if (keyboard->isButtonPressedRepeat(Keys::Backspace)) {
		if (caret > 0) {
			text.erase(text.begin() + (caret - 1));
			--caret;
			modified = true;
		}
	}

	if (keyboard->isButtonPressed(Keys::Enter) || keyboard->isButtonPressed(Keys::KP_Enter)) {
		if (!isMultiLine) {
			sendEvent(UIEvent(UIEventType::TextSubmit, getId(), getText()));
		}
	}

	if (keyboard->isButtonPressedRepeat(Keys::Home) || keyboard->isButtonPressedRepeat(Keys::PageUp)) {
		caret = 0;
	}
	if (keyboard->isButtonPressedRepeat(Keys::End) || keyboard->isButtonPressedRepeat(Keys::PageDown)) {
		caret = int(text.size());
	}

	int dx = (keyboard->isButtonPressedRepeat(Keys::Left) ? -1 : 0) + (keyboard->isButtonPressedRepeat(Keys::Right) ? 1 : 0);
	caret = clamp(caret + dx, 0, int(text.size()));

	for (int letter = keyboard->getNextLetter(); letter != 0; letter = keyboard->getNextLetter()) {
		if (!ctrlDown && letter >= 32) {
			if (!maxLength || int(text.size()) < maxLength.get()) {
				text.insert(text.begin() + caret, letter);
				caret++;
				modified = true;
			}
		}
	}

	setCaretPosition(caret);

	if (modified) {
		onTextModified();
	}
}

void UITextInput::setCaretPosition(int pos)
{
	pos = clamp(pos, 0, int(text.size()));
	if (pos != caretPos) {
		caretTime = 0;
		caretShowing = true;
		caretPos = pos;
		caretPhysicalPos = label.getCharacterPosition(caretPos, text).x;
	}
}

void UITextInput::onTextModified()
{
	if (getValidator()) {
		text = getValidator()->onTextChanged(text);
	}
	setCaretPosition(caretPos);

	const auto str = String(text);
	sendEvent(UIEvent(UIEventType::TextChanged, getId(), str));
	notifyDataBind(str);
}

void UITextInput::validateText()
{
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
}

void UITextInput::onValidatorSet()
{
	if (getValidator()) {
		text = getValidator()->onTextChanged(text);
	}
	setCaretPosition(caretPos);
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
	if (text.empty() && !isFocused()) {
		ghostText.checkForUpdates();
		label = style.getTextRenderer("labelGhost");
		label.setText(ghostText);
	} else {
		label = style.getTextRenderer("label");
		label.setText(text);
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
	if (keyboard) {
		while (keyboard->getNextLetter()) {}
	}
}

void UITextInput::pressMouse(Vector2f mousePos, int button)
{
	if (button == 0) {
		Vector2f labelClickPos = mousePos - label.getPosition();
		setCaretPosition(int(label.getCharacterAt(labelClickPos)));
	}
}

void UITextInput::readFromDataBind()
{
	setText(getDataBind()->getStringData());
}
