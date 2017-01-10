#include "ui/widgets/ui_image.h"

using namespace Halley;

UIImage::UIImage(Sprite s, Maybe<UISizer> sizer, Vector4f innerBorder)
	: UIWidget("", s.getScaledSize(), sizer, innerBorder)
	, sprite(s)
{
}

void UIImage::draw(UIPainter& painter) const
{
	painter.draw(sprite);

	UIWidget::draw(painter);
}

void UIImage::update(Time t)
{
	sprite.setPos(getPosition()).scaleTo(getSize());

	UIWidget::update(t);
}
