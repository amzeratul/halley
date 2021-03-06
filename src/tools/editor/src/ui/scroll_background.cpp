#include "scroll_background.h"
using namespace Halley;

ScrollBackground::ScrollBackground(String id, UIStyle style, UISizer sizer)
	: UIClickable(std::move(id), {}, std::move(sizer))
{
	bg = style.getSprite("background");
	
	setHandle(UIEventType::MouseWheel, [this] (const UIEvent& event)
	{
		onMouseWheel(event);
	});
}

float ScrollBackground::getZoomLevel() const
{
	return std::pow(2.0f, float(zoomExp));
}

void ScrollBackground::setZoomListener(ZoomListener listener)
{
	zoomListener = listener;
}

void ScrollBackground::setMousePosListener(MousePosListener listener)
{
	mousePosListener = listener;
}

void ScrollBackground::doSetState(State state)
{
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

	UIClickable::pressMouse(mousePos, button);
}

void ScrollBackground::releaseMouse(Vector2f mousePos, int button)
{
	if (button == 0) {
		if (dragging) {
			onMouseOver(mousePos);
			dragging = false;
		}
	}

	UIClickable::releaseMouse(mousePos, button);
}

void ScrollBackground::onMouseOver(Vector2f mousePos)
{
	lastMousePos = mousePos;
	if (dragging) {
		setDragPos(mouseStartPos - mousePos + startScrollPos);
	}

	if (mousePosListener) {
		mousePosListener(mousePos);
	}

	UIClickable::onMouseOver(mousePos);
}

void ScrollBackground::onDoubleClicked(Vector2f mousePos)
{
	sendEvent(UIEvent(UIEventType::ButtonDoubleClicked, getId()));
}

void ScrollBackground::setDragPos(Vector2f pos)
{
	pane->scrollTo(pos);
}

void ScrollBackground::onMouseWheel(const UIEvent& event)
{
	const float oldZoom = getZoomLevel();
	zoomExp = clamp(zoomExp + signOf(event.getIntData()), -5, 5);
	const float zoom = getZoomLevel();
	dirty = true;

	if (zoom != oldZoom) {
		pane = getScrollPane();
		if (!pane) {
			return;
		}

		const Vector2f childPos = getChildren().at(0)->getPosition() - getPosition();

		const Vector2f panelScrollPos = pane->getScrollPosition();

		if (zoomListener) {
			zoomListener(zoom);
		}

		pane->refresh();

		const Vector2f relMousePos = lastMousePos - pane->getPosition();
		const Vector2f oldMousePos = (relMousePos - childPos + panelScrollPos) / oldZoom;
		const Vector2f newScrollPos = oldMousePos * zoom - relMousePos;

		pane->scrollTo(newScrollPos);
	}
}

