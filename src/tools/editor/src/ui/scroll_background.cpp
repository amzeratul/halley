#include "scroll_background.h"

#include <utility>
using namespace Halley;

ScrollBackground::ScrollBackground(String id, UIStyle style, UISizer sizer, std::shared_ptr<InputKeyboard> keyboard)
	: BaseCanvas(std::move(id), std::move(style), std::move(sizer), std::move(keyboard))
{
}

UIScrollPane* ScrollBackground::getScrollPane() const
{
	return dynamic_cast<UIScrollPane*>(getParent());
}

void ScrollBackground::setScrollPosition(Vector2f pos)
{
	if (!pane) {
		pane = getScrollPane();
	}
	pane->scrollTo(pos);
}

Vector2f ScrollBackground::getScrollPosition() const
{
	if (!pane) {
		pane = getScrollPane();
	}
	return pane->getScrollPosition();
}

Vector2f ScrollBackground::getBasePosition() const
{
	if (!pane) {
		pane = getScrollPane();
	}
	return pane->getPosition();
}

void ScrollBackground::refresh()
{
	if (!pane) {
		pane = getScrollPane();
	}
	pane->refresh();
}

float ScrollBackground::getBackgroundScrollSpeed() const
{
	return 0.0f;
}

Vector2f ScrollBackground::getBackgroundOffset(Vector2f size) const
{
	return size * 0.5f;
}
