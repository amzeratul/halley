#include "ui/widgets/ui_input.h"
#include "ui/ui_style.h"
#include "ui/ui_validator.h"

using namespace Halley;

UIInput::UIInput(std::shared_ptr<InputDevice> keyboard, String id, std::shared_ptr<UIStyle> style, String text, String ghostText)
	: UIWidget(id, {}, UISizer(UISizerType::Vertical), Vector4f(3, 3, 3, 3))
	, keyboard(keyboard)
	, style(style)
	, sprite(style->getSprite("input.box"))
	, label(style->getTextRenderer("input.label"))
	, text(text.getUTF32())
	, ghostText(ghostText)
{
	label.setText(text);
}

bool UIInput::canInteractWithMouse() const
{
	return true;
}

UIInput& UIInput::setText(const String& t)
{
	text = t.getUTF32();
	return *this;
}

String UIInput::getText() const
{
	return String(text);
}

UIInput& UIInput::setGhostText(const String& t)
{
	ghostText = t;
	return *this;
}

String UIInput::getGhostText() const
{
	return ghostText;
}

void UIInput::draw(UIPainter& painter) const
{
	painter.draw(sprite);
	painter.draw(label);
}

void UIInput::updateTextInput()
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

	if (modified && getValidator()) {
		text = getValidator()->onTextChanged(text);
	}
}

void UIInput::update(Time t, bool moved)
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
		label = style->getTextRenderer("input.labelGhost");
		label.setText(ghostText);
	} else {
		label = style->getTextRenderer("input.label");
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

void UIInput::onFocus()
{
	caretTime = 0;
	caretShowing = true;
	while (keyboard->getNextLetter()) {}
}
