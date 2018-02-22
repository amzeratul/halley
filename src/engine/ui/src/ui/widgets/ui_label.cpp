#include "widgets/ui_label.h"

using namespace Halley;

UILabel::UILabel(TextRenderer text)
	: UIWidget("", {})
	, text(text)
{
	updateMinSize();
}

void UILabel::draw(UIPainter& painter) const
{
	if (needsClip) {
		painter.withClip(Rect4f(getPosition(), getPosition() + getMinimumSize())).draw(text);
	} else {
		painter.draw(text);
	}

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
	needsClip = false;
	auto extents = text.getExtents();
	if (extents.x > maxWidth) {
		text.setText(text.split(maxWidth));
		extents = text.getExtents();
	}
	if (extents.y > maxHeight) {
		extents.y = maxHeight;
		needsClip = true;
	}
	setMinSize(extents);
}

void UILabel::setText(const String& t)
{
	if (text.getText() != t) {
		text.setText(t);
		updateMinSize();
	}
}

void UILabel::setColourOverride(const std::vector<ColourOverride>& overrides)
{
	text.setColourOverride(overrides);
}

void UILabel::setMaxWidth(float m)
{
	maxWidth = m;
	updateMinSize();
}

void UILabel::setMaxHeight(float m)
{
	maxHeight = m;
	updateMinSize();
}

void UILabel::setTextRenderer(TextRenderer renderer)
{
	renderer.setText(text.getText()).setPosition(text.getPosition());
	text = renderer;
	updateMinSize();
}

void UILabel::setColour(Colour4f colour)
{
	text.setColour(colour);
}

void UILabel::setSelectable(Colour4f normalColour, Colour4f selColour)
{
	getEventHandler().setHandle(UIEventType::SetSelected, [=] (const UIEvent& event)
	{
		if (event.getBoolData()) {
			setColour(selColour);
		} else {
			setColour(normalColour);
		}
	});
}
