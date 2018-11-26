#include "widgets/ui_label.h"
#include "halley/text/i18n.h"

using namespace Halley;

UILabel::UILabel(const String& id, TextRenderer style, const LocalisedString& text)
	: UIWidget(id, {})
	, renderer(style)
	, text(text)
{
	updateText();
}

void UILabel::draw(UIPainter& painter) const
{
	if (needsClip) {
		painter.withClip(Rect4f(getPosition(), getPosition() + getMinimumSize())).draw(renderer);
	} else {
		painter.draw(renderer);
	}
}

void UILabel::update(Time t, bool moved)
{
	if (text.checkForUpdates()) {
		updateText();
	}
	if (moved) {
		renderer.setPosition(getPosition() + Vector2f(renderer.getAlignment() * textExtents.x, 0.0f));
	}
}

void UILabel::updateMinSize()
{
	needsClip = false;
	textExtents = renderer.getExtents();
	if (textExtents.x > maxWidth) {
		if (wordWrapped) {
			renderer.setText(renderer.split(maxWidth));
			textExtents = renderer.getExtents();
		} else {
			textExtents.x = maxWidth;
			needsClip = true;
		}
	}
	if (textExtents.y > maxHeight) {
		float maxLines = std::floor(maxHeight / renderer.getLineHeight());
		textExtents.y = maxLines * renderer.getLineHeight();
		needsClip = true;
	}
	setMinSize(textExtents);
}

void UILabel::updateText() {
	renderer.setText(text);
	updateMinSize();
}

void UILabel::setText(const LocalisedString& t)
{
	if (text != t) {
		text = t;
		updateText();
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

float UILabel::getMaxWidth() const
{
	return maxWidth;
}

float UILabel::getMaxHeight() const
{
	return maxHeight;
}

void UILabel::setWordWrapped(bool wrapped)
{
	wordWrapped = wrapped;
}

bool UILabel::isWordWrapped() const
{
	return wordWrapped;
}

bool UILabel::isClipped() const
{
	return needsClip;
}

void UILabel::setAlignment(float alignment)
{
	renderer.setAlignment(alignment);
}

TextRenderer& UILabel::getTextRenderer()
{
	return renderer;
}

const TextRenderer& UILabel::getTextRenderer() const
{
	return renderer;
}

Colour4f UILabel::getColour() const
{
	return renderer.getColour();
}

void UILabel::setTextRenderer(TextRenderer r)
{
	r.setText(renderer.getText()).setPosition(renderer.getPosition());
	renderer = r;
	updateMinSize();
}

void UILabel::setColour(Colour4f colour)
{
	renderer.setColour(colour);
}

void UILabel::setSelectable(Colour4f normalColour, Colour4f selColour)
{
	setHandle(UIEventType::SetSelected, [=] (const UIEvent& event)
	{
		if (event.getBoolData()) {
			setColour(selColour);
		} else {
			setColour(normalColour);
		}
	});
}

void UILabel::setDisablable(Colour4f normalColour, Colour4f disabledColour)
{
	setHandle(UIEventType::SetEnabled, [=] (const UIEvent& event)
	{
		if (event.getBoolData()) {
			setColour(normalColour);
		} else {
			setColour(disabledColour);
		}
	});
}
