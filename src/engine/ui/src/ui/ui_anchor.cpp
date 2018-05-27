#include "halley/ui/ui_anchor.h"
#include "ui_parent.h"
#include "ui_widget.h"
using namespace Halley;

UIAnchor::UIAnchor(Vector2f relativePos, Vector2f relativeAlignment, Vector2f absoluteOffset, Maybe<Rect4f> bounds)
	: relativePos(relativePos)
	, relativeAlignment(relativeAlignment)
	, absoluteOffset(absoluteOffset)
	, bounds(bounds)
{}

Vector2f UIAnchor::getRelativePos() const
{
	return relativePos;
}

Vector2f UIAnchor::getRelativeAlignment() const
{
	return relativeAlignment;
}

Vector2f UIAnchor::getAbsoluteOffset() const
{
	return absoluteOffset;
}

Maybe<Rect4f> UIAnchor::getBounds() const
{
	return bounds;
}

void UIAnchor::setBounds(UIParent& parent)
{
	bounds = parent.getRect();
}

void UIAnchor::setAutoBounds(bool enabled)
{
	autoBounds = enabled;
}

void UIAnchor::position(UIWidget& widget) const
{
	const auto size = widget.getSize();
	const auto targetRect = widget.getParent()->getRect();
	const Maybe<Rect4f> curBounds = autoBounds ? targetRect : bounds;
	const Vector2f anchorPos = targetRect.getTopLeft() + relativePos * targetRect.getSize();
	
	Vector2f targetPos = anchorPos - (size * relativeAlignment).floor() + absoluteOffset;
	if (curBounds) {
		targetPos.x = clamp(targetPos.x, curBounds->getLeft(), curBounds->getRight() - size.x);
		targetPos.y = clamp(targetPos.y, curBounds->getTop(), curBounds->getBottom() - size.y);
	}
	
	widget.setPosition(targetPos);
}
