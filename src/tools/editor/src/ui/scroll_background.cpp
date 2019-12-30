#include "scroll_background.h"
using namespace Halley;

ScrollBackground::ScrollBackground(String id, Resources& res, UISizer sizer)
	: UIWidget(std::move(id), {}, std::move(sizer))
{
	bg = Sprite()
		.setImage(res, "checkered.png")
		.setColour(Colour4f::fromString("#111111"));
}

void ScrollBackground::update(Time t, bool moved)
{
	if (moved) {
		bg
			.setPos(getPosition())
			.setSize(getSize())
			.setTexRect(Rect4f(Vector2f(), getSize() / Vector2f(16, 16)));
	}
}

void ScrollBackground::draw(UIPainter& painter) const
{
	painter.draw(bg);
}

bool ScrollBackground::canInteractWithMouse() const
{
	return true;
}

bool ScrollBackground::isFocusLocked() const
{
	return dragging;
}

void ScrollBackground::pressMouse(Vector2f mousePos, int button)
{
	if (button == 0) {
		pane = dynamic_cast<UIScrollPane*>(getParent());
		if (pane) {
			dragging = true;
			mouseStartPos = mousePos;
			startScrollPos = pane->getScrollPosition();
		}
	}
}

void ScrollBackground::releaseMouse(Vector2f mousePos, int button)
{
	if (button == 0) {
		if (dragging) {
			onMouseOver(mousePos);
			dragging = false;
		}
	}
}

void ScrollBackground::onMouseOver(Vector2f mousePos)
{
	if (dragging) {
		setDragPos(mouseStartPos - mousePos + startScrollPos);
	}
}

void ScrollBackground::setDragPos(Vector2f pos)
{
	pane->scrollTo(pos);
}

