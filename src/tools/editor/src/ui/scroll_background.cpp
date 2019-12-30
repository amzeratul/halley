#include "scroll_background.h"
using namespace Halley;

ScrollBackground::ScrollBackground(String id, Resources& res, UISizer sizer)
	: UIWidget(std::move(id), {}, std::move(sizer))
{
	bg = Sprite()
		.setImage(res, "checkered.png")
		.setColour(Colour4f::fromString("#111111"));

	setHandle(UIEventType::MouseWheel, [this] (const UIEvent& event)
	{
		onMouseWheel(event);
	});
}

float ScrollBackground::getZoomLevel() const
{
	return std::powf(2.0f, float(zoomExp));
}

void ScrollBackground::setZoomListener(ZoomListener listener)
{
	zoomListener = listener;
}

void ScrollBackground::update(Time t, bool moved)
{
	if (moved || dirty) {
		bg
			.setPos(getPosition())
			.setSize(getSize())
			.setTexRect(Rect4f(Vector2f(), getSize() / Vector2f(16, 16) / getZoomLevel()));
	}
	dirty = false;
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

UIScrollPane* ScrollBackground::getScrollPane() const
{
	return dynamic_cast<UIScrollPane*>(getParent());
}

void ScrollBackground::pressMouse(Vector2f mousePos, int button)
{
	if (button == 0) {
		pane = getScrollPane();
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
	lastMousePos = mousePos;
	if (dragging) {
		setDragPos(mouseStartPos - mousePos + startScrollPos);
	}
}

void ScrollBackground::setDragPos(Vector2f pos)
{
	pane->scrollTo(pos);
}

void ScrollBackground::onMouseWheel(const UIEvent& event)
{
	zoomExp = clamp(zoomExp + signOf(event.getIntData()), -5, 5);
	dirty = true;
	if (zoomListener) {
		zoomListener(getZoomLevel());
	}
}

