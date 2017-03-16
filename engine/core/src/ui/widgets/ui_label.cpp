#include "ui/widgets/ui_label.h"

using namespace Halley;

UILabel::UILabel(TextRenderer text)
	: UIWidget("", {})
	, text(text)
{
	updateMinSize();
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

void UILabel::updateMinSize()
{
	auto extents = text.getExtents();
	if (extents.x > maxWidth) {
		text.setText(text.split(maxWidth));
		extents = text.getExtents();
	}
	setMinSize(extents);
}

void UILabel::setText(const String& t)
{
	text.setText(t);
	updateMinSize();
}

void UILabel::setMaxWidth(float m)
{
	maxWidth = m;
	updateMinSize();
}

void UILabel::setColour(Colour4f colour)
{
	text.setColour(colour);
}
