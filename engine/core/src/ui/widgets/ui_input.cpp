#include "ui/widgets/ui_input.h"
#include "ui/ui_style.h"

using namespace Halley;

UIInput::UIInput(String id, std::shared_ptr<UIStyle> style, String text, String ghostText)
	: UIWidget(id, {}, UISizer(UISizerType::Vertical), Vector4f(3, 3, 3, 3))
	, style(style)
	, sprite(style->inputBox)
	, label(style->inputLabel)
	, text(text)
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
	text = t;
	return *this;
}

String UIInput::getText() const
{
	return text;
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
	if (text.isEmpty()) {
		label = style->inputLabelGhost;
		label.setText(ghostText);
	} else {
		label = style->inputLabel;
		label.setText(text);
	}
	label.setPosition(getPosition() + Vector2f(3, 0));

	if (moved) {
		sprite.setPos(getPosition()).scaleTo(getSize());
	}
}
