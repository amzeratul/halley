#include "popup_window.h"
using namespace Halley;

PopupWindow::PopupWindow(String id)
	: UIWidget(std::move(id), {}, UISizer())
{
	setModal(true);
	setAnchor(UIAnchor());
	setInteractWithMouse(true);
	setMouseBlocker(true);
}

void PopupWindow::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	const auto grabArea = Rect4f(getPosition(), getPosition() + Vector2f(getSize().x, 20));
	if (button == 0 && grabArea.contains(mousePos)) {
		dragging = true;
		startDragPos = mousePos - getPosition();
	}
}

void PopupWindow::releaseMouse(Vector2f mousePos, int button)
{
	if (button == 0) {
		dragging = false;
	}
}

void PopupWindow::onMouseOver(Vector2f mousePos)
{
	if (dragging) {
		const Rect4f rootBounds = getRoot()->getRect();
		const Rect4f bounds = Rect4f(rootBounds.getTopLeft(), rootBounds.getBottomRight() - getSize());
		setAnchor(UIAnchor(Vector2f(), Vector2f(), bounds.getClosestPoint(mousePos - startDragPos)));
	}
}

bool PopupWindow::isFocusLocked() const
{
	return dragging;
}
