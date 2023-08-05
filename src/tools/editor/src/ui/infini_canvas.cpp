#include "infini_canvas.h"
using namespace Halley;

InfiniCanvas::InfiniCanvas(String id, UIStyle style, UISizer sizer, std::shared_ptr<InputKeyboard> keyboard)
	: BaseCanvas(std::move(id), std::move(style), std::move(sizer), std::move(keyboard))
{
}

void InfiniCanvas::setScrollPosition(Vector2f pos)
{
	const auto zoom = getZoomLevel();
	auto newPos = (pos * zoom).round() / zoom;
	if (scrollPos != newPos) {
		scrollPos = newPos;
		onNewScrollPosition(scrollPos);
	}
}

Vector2f InfiniCanvas::getScrollPosition() const
{
	return scrollPos;
}

Vector2f InfiniCanvas::getBasePosition() const
{
	return getPosition() + getSize() / 2;
}

Vector2f InfiniCanvas::getLayoutOriginPosition() const
{
	return getPosition() + getSize() / 2 - scrollPos;
}

Vector2f InfiniCanvas::getLayoutMinimumSize(bool force) const
{
	return getMinimumSize();
}

void InfiniCanvas::drawChildren(UIPainter& painter) const
{
	auto p = painter.withClip(getRect());
	UIWidget::drawChildren(p);
}
