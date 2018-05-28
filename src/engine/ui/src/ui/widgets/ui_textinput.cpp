#include "widgets/ui_textinput.h"
#include "ui_style.h"
#include "ui_validator.h"
#include "halley/text/i18n.h"
#include "halley/ui/ui_data_bind.h"
#include "halley/support/logger.h"
#include "halley/core/input/input_keys.h"

using namespace Halley;

UITextInput::UITextInput(std::shared_ptr<InputDevice> keyboard, String id, UIStyle style, String text, LocalisedString ghostText)
	: UIWidget(id, {}, UISizer(UISizerType::Vertical), style.getBorder("innerBorder"))
	, keyboard(keyboard)
	, style(style)
	, sprite(style.getSprite("box"))
	, caret(style.getSprite("caret"))
	, label(style.getTextRenderer("label"))
	, text(text.getUTF32())
	, ghostText(ghostText)
{
	label.setText(text);
}

bool UITextInput::canInteractWithMouse() const
{
	return true;
}

UITextInput& UITextInput::setText(const String& t)
{
	text = t.getUTF32();
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
	bool modified = false;
	int startCaret = caretPos;

	if (keyboard->isButtonPressedRepeat(Keys::Delete)) {
		if (caretPos < int(text.size())) {
			text.erase(text.begin() + caretPos);
			modified = true;
		}
	}
	
	if (keyboard->isButtonPressedRepeat(Keys::Backspace)) {
		if (caretPos > 0) {
			text.erase(text.begin() + (caretPos - 1));
			--caretPos;
			modified = true;
		}
	}

	int dx = (keyboard->isButtonPressedRepeat(Keys::Left) ? -1 : 0) + (keyboard->isButtonPressedRepeat(Keys::Right) ? 1 : 0);
	caretPos = clamp(caretPos + dx, 0, int(text.size()));

	for (int letter = keyboard->getNextLetter(); letter != 0; letter = keyboard->getNextLetter()) {
		if (letter >= 32) {
			text.insert(text.begin() + caretPos, letter);
			caretPos++;
			modified = true;
		}
	}

	if (caretPos != startCaret || modified) {
		caretPhysicalPos = label.getCharacterPosition(caretPos, text).x;
	}

	if (modified) {
		if (getValidator()) {
			text = getValidator()->onTextChanged(text);
		}
		const auto str = String(text);
		sendEvent(UIEvent(UIEventType::TextChanged, getId(), str));
		notifyDataBind(str);
	}
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

	// Draw label
	const float length = label.getExtents().x;
	const float capacityX = getSize().x - getInnerBorder().x - getInnerBorder().z;
	const float capacityY = getSize().y - getInnerBorder().y - getInnerBorder().w;
	const Vector2f startPos = getPosition() + Vector2f(getInnerBorder().x, getInnerBorder().y);
	Vector2f textStart = startPos;
	if (length > capacityX) {
		textStart += Vector2f(capacityX - length, 0);
		label
			.setClip(Rect4f(Vector2f(length - capacityX, 0), Vector2f(length, capacityY)))
			.setPosition(textStart);
	} else {
		label
			.setClip()
			.setPosition(startPos);
	}
	caret.setPos(textStart + Vector2f(caretPhysicalPos, 0));

	if (moved) {
		sprite.setPos(getPosition()).scaleTo(getSize());
	}
}

void UITextInput::onFocus()
{
	caretTime = 0;
	caretShowing = true;
	while (keyboard->getNextLetter()) {}
}

void UITextInput::readFromDataBind()
{
	setText(getDataBind()->getStringData());
}
