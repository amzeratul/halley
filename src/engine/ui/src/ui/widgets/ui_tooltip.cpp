#include "widgets/ui_tooltip.h"

using namespace Halley;

UIToolTip::UIToolTip(const UIStyle& style)
	: UIWidget("tooltip", {}, UISizer())
{
	background = style.getSprite("background");
	text = style.getTextRenderer("label");
	border = style.getBorder("innerBorder");
	delay = style.getFloat("delay");
}

void UIToolTip::showToolTipForWidget(const UIWidget& widget, Vector2f mousePos)
{
	lastMousePos = mousePos;
	
	if (curWidget == &widget) {
		return;
	}
	
	hide();
	curWidget = &widget;

	const auto& toolTipText = widget.getToolTip();
	if (!toolTipText.isEmpty()) {
		setActive(true);
		text.setText(toolTipText);
		const auto size = text.getExtents();
		setMinSize(size + border.xy() + border.zw());
	}
}

void UIToolTip::hide()
{
	setActive(false);
	curWidget = nullptr;
	timeOnWidget = 0;
	visible = false;
}

void UIToolTip::update(Time t, bool moved)
{
	if (curWidget) {
		timeOnWidget += t;
		if (timeOnWidget > delay && !visible) {
			visible = true;

			auto pos = lastMousePos + Vector2f(0, 20);
			const auto screenRect = getRoot()->getRect();
			pos = Vector2f::max(pos, screenRect.getTopLeft());
			pos = Vector2f::min(pos, screenRect.getBottomRight() - getSize());
			
			setPosition(pos);
		}
	}
	text.setPosition(getPosition() + border.xy());
	background.setPosition(getPosition()).scaleTo(getSize());
}

void UIToolTip::draw(UIPainter& painter) const
{
	if (visible) {
		auto p2 = painter.withAdjustedLayer(100);
		p2.draw(background);
		p2.draw(text);
	}
}
