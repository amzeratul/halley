#include "base_canvas.h"

#include <utility>
using namespace Halley;

BaseCanvas::BaseCanvas(String id, UIStyle style, UISizer sizer, std::shared_ptr<InputKeyboard> keyboard)
	: UIClickable(std::move(id), {}, std::move(sizer))
	, keyboard(std::move(keyboard))
{
	bg = style.getSprite("background");
	draggingButton.fill(false);
	
	setHandle(UIEventType::MouseWheel, [this] (const UIEvent& event)
	{
		onMouseWheel(event);
	});
}

float BaseCanvas::getZoomLevel() const
{
	return std::pow(2.0f, float(zoomExp));
}

void BaseCanvas::setZoomListener(ZoomListener listener)
{
	zoomListener = listener;
}

void BaseCanvas::setMousePosListener(MousePosListener listener)
{
	mousePosListener = listener;
}

void BaseCanvas::setZoomEnabled(bool enabled)
{
	zoomEnabled = enabled;
}

void BaseCanvas::doSetState(State state)
{
}

void BaseCanvas::update(Time t, bool moved)
{
	if (moved || dirty) {
		bg
			.setPos(getPosition())
			.setSize(getSize())
			.setTexRect(Rect4f(Vector2f(), getSize() / Vector2f(16, 16) / getZoomLevel()));
	}
	dirty = false;

	UIClickable::update(t, moved);
}

void BaseCanvas::draw(UIPainter& painter) const
{
	painter.draw(bg);
}

void BaseCanvas::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	const bool needSpaceForLeftClick = false;
	const bool canLeftClickScroll = !needSpaceForLeftClick || keyboard && keyboard->isButtonDown(KeyCode::Space);
	if (button == 0 && canLeftClickScroll) {
		draggingButton[0] = true;
	}
	if (button == 1) {
		draggingButton[1] = true;
	}
	const bool shouldDrag = draggingButton[0] || draggingButton[1];

	if (shouldDrag && !dragging) {
		dragging = true;
		mouseStartPos = mousePos;
		startScrollPos = getScrollPosition();
	}

	UIClickable::pressMouse(mousePos, button, keyMods);
}

void BaseCanvas::releaseMouse(Vector2f mousePos, int button)
{
	if (button == 0 || button == 1) {
		draggingButton[button] = false;
	}
	const bool shouldDrag = draggingButton[0] || draggingButton[1];

	if (dragging && !shouldDrag) {
		onMouseOver(mousePos);
		dragging = false;
	}

	UIClickable::releaseMouse(mousePos, button);
}

void BaseCanvas::onMouseOver(Vector2f mousePos)
{
	lastMousePos = mousePos;
	if (dragging) {
		setScrollPosition(mouseStartPos - mousePos + startScrollPos);
	}

	if (mousePosListener) {
		mousePosListener(mousePos);
	}

	UIClickable::onMouseOver(mousePos);
}

void BaseCanvas::onDoubleClicked(Vector2f mousePos, KeyMods keyMods)
{
	sendEvent(UIEvent(UIEventType::ButtonDoubleClicked, getId()));
}

void BaseCanvas::refresh()
{
}

void BaseCanvas::onMouseWheel(const UIEvent& event)
{
	if (!zoomEnabled) {
		return;
	}
	
	const float oldZoom = getZoomLevel();
	zoomExp = clamp(zoomExp + signOf(event.getIntData()), -5, 5);
	const float zoom = getZoomLevel();
	dirty = true;

	if (zoom != oldZoom) {
		const Vector2f childPos = getChildren().at(0)->getPosition() - getPosition();

		const Vector2f panelScrollPos = getScrollPosition();

		if (zoomListener) {
			zoomListener(zoom);
		}

		refresh();

		const Vector2f relMousePos = lastMousePos - getBasePosition();
		const Vector2f oldMousePos = (relMousePos - childPos + panelScrollPos) / oldZoom;
		const Vector2f newScrollPos = oldMousePos * zoom - relMousePos;

		setScrollPosition(newScrollPos);
	}
}

