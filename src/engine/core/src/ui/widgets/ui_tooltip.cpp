#include "halley/ui/widgets/ui_tooltip.h"

using namespace Halley;

UIToolTip::UIToolTip(const UIStyle& style)
	: UIWidget("tooltip", {}, UISizer())
{
	background = style.getSprite("background");
	text = style.getTextRenderer("label");
	border = style.getBorder("innerBorder");
	delay = style.getTime("delay");
	maxWidth = style.getFloat("maxWidth");

	setModal(false);
}

void UIToolTip::showToolTipForWidget(const UIWidget& widget, Vector2f mousePos)
{
	displayPos = widget.getToolTipPosition(mousePos);
	
	if (curWidget == &widget && !widget.hasDynamicToolTip()) {
		return;
	}

	const auto& toolTipText = widget.getToolTip();
	if (curWidget != &widget || widget.hasDynamicToolTip() && toolTipText != curToolTipText) {
		hide();
		curWidget = &widget;
	}

	curToolTipText = toolTipText;
	if (curToolTipText.getString().isEmpty()) {
		setActive(false);
	} else {
		setActive(true);
		text.setText(text.split(curToolTipText.getString(), maxWidth));
		const auto size = text.getExtents();
		setMinSize(size + border.xy() + border.zw());

		if (widget.hasDynamicToolTip()) {
			positionAt(displayPos);
		}
		layout();
	}
}

void UIToolTip::hide()
{
	setActive(false);
	curWidget = nullptr;
	curToolTipText = {};
	timeOnWidget = 0;
	visible = false;
}

void UIToolTip::update(Time t, bool moved)
{
	if (curWidget) {
		timeOnWidget += t;
		if (timeOnWidget > delay && !visible) {
			visible = true;
			positionAt(displayPos);
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

void UIToolTip::positionAt(Vector2f pos)
{
	const auto screenRect = getRoot()->getRect();
	pos = Vector2f::max(pos, screenRect.getTopLeft());
	pos = Vector2f::min(pos, screenRect.getBottomRight() - getSize());
	
	setPosition(pos);
}
