#include "infini_canvas.h"
using namespace Halley;

InfiniCanvas::InfiniCanvas(String id, UIStyle style, UISizer sizer, std::shared_ptr<InputKeyboard> keyboard)
	: BaseCanvas(std::move(id), std::move(style), std::move(sizer), std::move(keyboard))
{
}

void InfiniCanvas::setScrollPosition(Vector2f pos)
{
	scrollPos = pos;
	Logger::logInfo(toString(pos));
}

Vector2f InfiniCanvas::getScrollPosition() const
{
	return scrollPos;
}

Vector2f InfiniCanvas::getBasePosition() const
{
	return getPosition();
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
