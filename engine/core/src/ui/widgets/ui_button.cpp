#include "ui/widgets/ui_button.h"
#include "ui/ui_style.h"

using namespace Halley;

UIButton::UIButton(String id, std::shared_ptr<UIStyle> s)
	: UIWidget(id, {}, UISizer(UISizerType::Horizontal, 1), s->buttonInnerBorder)
	, style(s)
{
	sprite = style->buttonNormal;
}

void UIButton::draw(UIPainter& painter) const
{
	painter.draw(sprite);

	UIWidget::draw(painter);
}

void UIButton::update(Time t)
{
	if (held) {
		if (mouseOver) {
			sprite = style->buttonDown;
		} else {
			sprite = style->buttonHover;
		}
	} else {
		if (isFocused()) {
			sprite = style->buttonHover;
		} else {
			sprite = style->buttonNormal;
		}
	}
	sprite.scaleTo(getSize()).setPos(getPosition());

	UIWidget::update(t);
}

bool UIButton::isFocusable() const
{
	return true;
}

bool UIButton::isFocusLocked() const
{
	return held;
}

void UIButton::pressMouse(int button)
{
	if (button == 0) {
		held = true;
	}
}

void UIButton::releaseMouse(int button)
{
	if (button == 0) {
		held = false;
	}
}
