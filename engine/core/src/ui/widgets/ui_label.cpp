#include "ui/widgets/ui_label.h"

using namespace Halley;

UILabel::UILabel(TextRenderer text)
	: UIWidget("", text.getExtents())
	, text(text)
{
}

void UILabel::draw(UIPainter& painter) const
{
	painter.draw(text);

	UIWidget::draw(painter);
}

void UILabel::update(Time t)
{
	text.setPosition(getPosition());
}
