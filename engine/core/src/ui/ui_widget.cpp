#include "ui/ui_widget.h"

using namespace Halley;

UIWidget::UIWidget(String id, Vector2f minSize, Maybe<UISizer> sizer, Vector4f innerBorder)
	: id(id)
	, size(size)
	, minSize(minSize)
	, innerBorder(innerBorder)
	, sizer(sizer)
{
}

Vector2f UIWidget::computeMinimumSize()
{
	Vector2f minSize = getMinimumSize();
	auto sizer = getSizer();
	if (sizer) {
		auto border = getInnerBorder();
		Vector2f innerSize = sizer.get().computeMinimumSize();
		if (innerSize.x > 0.1f || innerSize.y > 0.1f) {
			innerSize += Vector2f(border.x + border.z, border.y + border.w);
		}
		return Vector2f::max(minSize, innerSize);
	}
	return minSize;
}

void UIWidget::setRect(Rect4f rect)
{
	setWidgetRect(rect);
	auto sizer = getSizer();
	if (sizer) {
		auto border = getInnerBorder();
		sizer.get().setRect(Rect4f(Vector2f(border.x, border.y), rect.getSize() - Vector2f(border.z, border.w)));
	}
}

Maybe<UISizer>& UIWidget::getSizer()
{
	return sizer;
}

const Maybe<UISizer>& UIWidget::getSizer() const
{
	return sizer;
}

bool UIWidget::isFocusable() const
{
	return focusable;
}

String UIWidget::getId() const
{
	return id;
}

Vector2f UIWidget::getMinimumSize() const
{
	return minSize;
}

Vector4f UIWidget::getInnerBorder() const
{
	return innerBorder;
}

void UIWidget::setWidgetRect(Rect4f rect)
{
	position = rect.getTopLeft();
	size = rect.getSize();
}
