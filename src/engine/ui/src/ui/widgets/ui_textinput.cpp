#include "widgets/ui_textinput.h"
#include "ui_style.h"
#include "ui_validator.h"
#include "halley/text/i18n.h"

using namespace Halley;

UITextInput::UITextInput(std::shared_ptr<InputDevice> keyboard, String id, UIStyle style, String text, LocalisedString ghostText)
	: UIWidget(id, {}, UISizer(UISizerType::Vertical), Vector4f(3, 3, 3, 3))
	, keyboard(keyboard)
	, style(style)
	, sprite(style.getSprite("box"))
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
}

void UITextInput::updateTextInput()
{
	bool modified = false;
	for (int letter = keyboard->getNextLetter(); letter != 0; letter = keyboard->getNextLetter()) {
		if (letter == 8) { // Backspace
			if (!text.empty()) {
				text.pop_back();
				modified = true;
			}
		} else if (letter >= 32) {
			text.push_back(letter);
			modified = true;
		}
	}

	if (modified) {
		if (getValidator()) {
			text = getValidator()->onTextChanged(text);
		}
		sendEvent(UIEvent(UIEventType::TextChanged, getId(), String(text)));
	}
}

void UITextInput::update(Time t, bool moved)
{
	caretTime += float(t);
	if (caretTime > 0.4f) {
		caretTime -= 0.4f;
		caretShowing = !caretShowing;
	}

	if (isFocused()) {
		updateTextInput();
	}

	if (text.empty() && !isFocused()) {
		label = style.getTextRenderer("labelGhost");
		label.setText(ghostText);
	} else {
		label = style.getTextRenderer("label");
		auto txt = String(text);
		if (isFocused()) {
			txt.appendCharacter(caretShowing ? '_' : ' ');
		}
		label.setText(txt);
	}

	float length = label.getExtents().x;
	float capacity = getSize().x - 6;
	if (length > capacity) {
		label
			.setAlignment(1.0f)
			.setClip(Rect4f(Vector2f(-capacity, 0), Vector2f(capacity, 20)))
			.setPosition(getPosition() + Vector2f(3 + capacity, 0));
	} else {
		label
			.setAlignment(0.0f)
			.setClip()
			.setPosition(getPosition() + Vector2f(3, 0));
	}

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
