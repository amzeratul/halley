#include "ui/widgets/ui_input.h"
#include "ui/ui_style.h"

using namespace Halley;

UIInput::UIInput(std::shared_ptr<InputKeyboard> keyboard, String id, std::shared_ptr<UIStyle> style, String text, String ghostText)
	: UIWidget(id, {}, UISizer(UISizerType::Vertical), Vector4f(3, 3, 3, 3))
	, keyboard(keyboard)
	, style(style)
	, sprite(style->inputBox)
	, label(style->inputLabel)
	, text(text.getUTF32())
	, ghostText(ghostText)
{
	label.setText(text);
}

bool UIInput::isFocusable() const
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

void UIInput::update(Time t, bool moved)
{
	caretTime += float(t);
	if (caretTime > 0.4f) {
		caretTime -= 0.4f;
		caretShowing = !caretShowing;
	}

	if (isFocused()) {
		for (int letter = keyboard->getNextLetter(); letter != 0; letter = keyboard->getNextLetter()) {
			if (letter == 8) { // Backspace
				if (!text.empty()) {
					text.pop_back();
				}
			} else if (letter >= 32) {
				text.push_back(letter);
			}
		}
	}

	if (text.empty() && !isFocused()) {
		label = style->inputLabelGhost;
		label.setText(ghostText);
	} else {
		label = style->inputLabel;
		auto txt = String(text);
		if (isFocused() && caretShowing) {
			txt.appendCharacter('|');
		}
		label.setText(txt);
	}
	label.setPosition(getPosition() + Vector2f(3, 0));

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
