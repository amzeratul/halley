#include "ui/widgets/ui_input.h"
#include "ui/ui_style.h"

using namespace Halley;

UIInput::UIInput(String id, std::shared_ptr<UIStyle> style)
	: UIWidget(id, {}, UISizer(UISizerType::Vertical), Vector4f(3, 3, 3, 3))
	, style(style)
	, sprite(style->inputBox)
	, label(style->inputLabel)
{
	label.setText("32");
}

bool UIInput::isFocusable() const
{
	return true;
}

void UIInput::draw(UIPainter& painter) const
{
	painter.draw(sprite);
	painter.draw(label);
}

void UIInput::update(Time t, bool moved)
{
	if (moved) {
		sprite.setPos(getPosition()).scaleTo(getSize());
		label.setPosition(getPosition() + Vector2f(3, 0));
	}
}
