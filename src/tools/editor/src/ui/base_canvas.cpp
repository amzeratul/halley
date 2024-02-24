#include "base_canvas.h"

#include <utility>
using namespace Halley;

BaseCanvas::BaseCanvas(String id, UIStyle style, UISizer sizer, std::shared_ptr<InputKeyboard> keyboard)
	: UIClickable(std::move(id), {}, std::move(sizer))
	, keyboard(std::move(keyboard))
	, draggingButton({false, false})
{
	bg = style.getSprite("background");
	bgSize = bg.getSize();
	border = style.getSprite("border");
	
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

void BaseCanvas::setScrollListener(ScrollListener listener)
{
	scrollListener = listener;
}

void BaseCanvas::setZoomEnabled(bool enabled)
{
	zoomEnabled = enabled;
}

void BaseCanvas::setScrollEnabled(bool enabled)
{
	scrollEnabled = enabled;
}

void BaseCanvas::setLeftClickScrollEnabled(bool enabled)
{
	leftClickScrollEnabled = enabled;
}

void BaseCanvas::setLeftClickScrollKey(std::optional<KeyCode> key)
{
	leftClickScrollKey = key;
}

void BaseCanvas::setMouseMirror(std::shared_ptr<UIWidget> widget, bool evenWhenDragging)
{
	mouseMirror = std::move(widget);
	mirrorWhenDragging = evenWhenDragging;
}

std::optional<Vector2f> BaseCanvas::transformToChildSpace(Vector2f pos) const
{
	if (getRect().contains(pos)) {
		return pos;
	} else {
		return std::nullopt;
	}
}

void BaseCanvas::doSetState(State state)
{
}

void BaseCanvas::update(Time t, bool moved)
{
	const auto scale = Vector2f(1, 1) / (bgSize * getZoomLevel());

	const auto centrePos = getScrollPosition() * scale * getBackgroundScrollSpeed();
	const auto size = getSize() * scale;

	bg
		.setPos(getPosition())
		.setSize(getSize())
		.setTexRect(Rect4f(centrePos - 0.5f * size, centrePos + 0.5f * size) + getBackgroundOffset(size));

	border
		.setPos(getPosition())
		.scaleTo(getSize());

	UIClickable::update(t, moved);
}

void BaseCanvas::draw(UIPainter& painter) const
{
	painter.draw(bg);
}

void BaseCanvas::drawAfterChildren(UIPainter& painter) const
{
	painter.draw(border);
}

void BaseCanvas::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	const bool canLeftClickScroll = leftClickScrollEnabled && (!leftClickScrollKey || keyboard && keyboard->isButtonDown(*leftClickScrollKey));
	if (button == 0 && canLeftClickScroll) {
		draggingButton[0] = true;
	}
	if (button == 1) {
		draggingButton[1] = true;
	}
	const bool shouldDrag = scrollEnabled && (draggingButton[0] || draggingButton[1]);

	if (shouldDrag && !dragging) {
		dragging = true;
		mouseStartPos = mousePos;
		startScrollPos = getScrollPosition();
	}

	UIClickable::pressMouse(mousePos, button, keyMods);

	if (mouseMirror && (!dragging || mirrorWhenDragging)) {
		mouseMirror->pressMouse(mousePos, button, keyMods);
	}
}

void BaseCanvas::releaseMouse(Vector2f mousePos, int button)
{
	if (button == 0 || button == 1) {
		draggingButton[button] = false;
	}
	const bool shouldDrag = scrollEnabled && (draggingButton[0] || draggingButton[1]);

	UIClickable::releaseMouse(mousePos, button);

	if (mouseMirror && !dragging) {
		mouseMirror->releaseMouse(mousePos, button);
	}
	
	if (dragging && !shouldDrag) {
		onMouseOver(mousePos, KeyMods::None); // Hmmm
		dragging = false;
	}
}

void BaseCanvas::onMouseOver(Vector2f mousePos, KeyMods keyMods)
{
	lastMousePos = mousePos;
	if (dragging) {
		setScrollPosition(mouseStartPos - mousePos + startScrollPos);
	}

	if (mousePosListener) {
		mousePosListener(mousePos, keyMods);
	}

	UIClickable::onMouseOver(mousePos);

	if (mouseMirror) {
		if (dragging) {
			mouseMirror->onMouseLeft(mousePos);
		} else {
			mouseMirror->onMouseOver(mousePos);
		}
	}
}

void BaseCanvas::onDoubleClicked(Vector2f mousePos, KeyMods keyMods)
{
	if (mouseMirror) {
		mouseMirror->sendEvent(UIEvent(UIEventType::CanvasDoubleClicked, getId()));
	} else {
		sendEvent(UIEvent(UIEventType::CanvasDoubleClicked, getId()));
	}
}

void BaseCanvas::onNewScrollPosition(Vector2f pos) const
{
	if (scrollListener) {
		scrollListener(pos);
	}
}

float BaseCanvas::getBackgroundScrollSpeed() const
{
	return 1.0f;
}

Vector2f BaseCanvas::getBackgroundOffset(Vector2f size) const
{
	return Vector2f(0.5f, 0.5f);
}

void BaseCanvas::refresh()
{
}

void BaseCanvas::onMouseWheel(const UIEvent& event)
{
	if (mouseMirror) {
		mouseMirror->sendEventDown(event);
	}

	changeZoom(signOf(event.getIntData()), lastMousePos);
}

void BaseCanvas::changeZoom(int amount, std::optional<Vector2f> anchor)
{
	if (!zoomEnabled) {
		return;
	}
	
	const float zoom0 = getZoomLevel();
	zoomExp = clamp(zoomExp + amount, -5, 5);
	const float zoom1 = getZoomLevel();

	if (zoom1 != zoom0) {
		const Vector2f scrollPos0 = getScrollPosition();

		if (zoomListener) {
			zoomListener(zoom1);
		}

		refresh();

		if (anchor) {
			const Vector2f relMousePos = *anchor - getBasePosition();
			const Vector2f scrollPos1 = (relMousePos + scrollPos0) / zoom0 * zoom1 - relMousePos;

			setScrollPosition(scrollPos1);
		}
	}
}

