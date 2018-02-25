#include "widgets/ui_label.h"
#include "halley/text/i18n.h"

using namespace Halley;

UILabel::UILabel(TextRenderer style, const LocalisedString& text)
	: UIWidget("", {})
	, renderer(style)
	, text(text)
{
	renderer.setText(text);
	updateMinSize();
}

void UILabel::draw(UIPainter& painter) const
{
	if (needsClip) {
		painter.withClip(Rect4f(getPosition(), getPosition() + getMinimumSize())).draw(renderer);
	} else {
		painter.draw(renderer);
	}

	UIWidget::draw(painter);
}

void UILabel::update(Time t, bool moved)
{
	if (moved) {
		renderer.setPosition(getPosition());
	}
}

void UILabel::updateMinSize()
{
	needsClip = false;
	auto extents = renderer.getExtents();
	if (extents.x > maxWidth) {
		renderer.setText(renderer.split(maxWidth));
		extents = renderer.getExtents();
	}
	if (extents.y > maxHeight) {
		extents.y = maxHeight;
		needsClip = true;
	}
	setMinSize(extents);
}

void UILabel::setText(const LocalisedString& t)
{
	if (text != t) {
		text = t;
		renderer.setText(text);
		updateMinSize();
	}
}

void UILabel::setColourOverride(const std::vector<ColourOverride>& overrides)
{
	renderer.setColourOverride(overrides);
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
	renderer.setText(renderer.getText()).setPosition(renderer.getPosition());
	renderer = renderer;
	updateMinSize();
}

void UILabel::setColour(Colour4f colour)
{
	renderer.setColour(colour);
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
