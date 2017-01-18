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

void UILabel::update(Time t, bool moved)
{
	if (moved) {
		text.setPosition(getPosition());
	}
}

void UILabel::setText(const String& t)
{
	text.setText(t);
	setMinSize(text.getExtents());
}
