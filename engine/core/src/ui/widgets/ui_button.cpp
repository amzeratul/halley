#include "ui/widgets/ui_button.h"
#include "ui/ui_style.h"

using namespace Halley;

UIButton::UIButton(String id, std::shared_ptr<UIStyle> style)
	: UIWidget(id, {}, UISizer(UISizerType::Horizontal, 1))
	, style(style)
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
	sprite.scaleTo(getSize()).setPos(getPosition());

	UIWidget::update(t);
}
