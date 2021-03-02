#include "widgets/ui_clickable.h"

#include "halley/support/logger.h"

using namespace Halley;

UIClickable::UIClickable(String id, Vector2f minSize, std::optional<UISizer> sizer, Vector4f innerBorder)
	: UIWidget(std::move(id), minSize, std::move(sizer), innerBorder)
{
}

bool UIClickable::canInteractWithMouse() const
{
	return true;
}

bool UIClickable::isFocusLocked() const
{
	return held[0];
}

void UIClickable::pressMouse(Vector2f, int button)
{
	if (isEnabled()) {
		held[button] = true;
	}
}

void UIClickable::releaseMouse(Vector2f mousePos, int button)
{
	if (isEnabled()) {
		if (held[button] && isMouseOver()) {
			onMouseClicked(mousePos, button);

			if (clickTime[button] < 0.5 && (mousePos - clickPos[button]).length() < 3.0f) {
				onMouseDoubleClicked(mousePos, button);
			}

			clickTime[button] = 0;
			clickPos[button] = mousePos;
		}
		held[button] = false;
	}
}

void UIClickable::onClick(UIEventCallback callback)
{
	setHandle(UIEventType::ButtonClicked, std::move(callback));
}

UIClickable::State UIClickable::getCurState() const
{
	return curState;
}

bool UIClickable::updateButton()
{
	bool dirty = false;
	if (held[0]) {
		if (isMouseOver()) {
			dirty |= setState(State::Down);
		} else {
			dirty |= setState(State::Hover);
		}
	} else {
		if (isMouseOver()) {
			dirty |= setState(State::Hover);
		} else {
			dirty |= setState(State::Up);
		}
	}
	forceUpdate = false;
	return dirty;
}

void UIClickable::doForceUpdate()
{
	forceUpdate = true;
}

void UIClickable::onClicked(Vector2f mousePos)
{
}

void UIClickable::onDoubleClicked(Vector2f mousePos)
{
}

void UIClickable::onRightClicked(Vector2f mousePos)
{
}

void UIClickable::onRightDoubleClicked(Vector2f mousePos)
{
}

void UIClickable::onMiddleClicked(Vector2f mousePos)
{
}

void UIClickable::onGamepadInput(const UIInputResults& input, Time time)
{
	if (input.isButtonPressed(UIGamepadInput::Button::Accept)) {
		onShortcutPressed();
		onClicked(Vector2f());
	}
}

Rect4f UIClickable::getMouseRect() const
{
	auto rect = UIWidget::getMouseRect();

	if (mouseExtraBorder) {
		const auto& b = mouseExtraBorder.value();
		return Rect4f(rect.getTopLeft() - Vector2f(b.x, b.y), rect.getBottomRight() + Vector2f(b.z, b.w));
	}

	return rect;
}

void UIClickable::onStateChanged(State prev, State next)
{
}

bool UIClickable::setState(State state)
{
	addNewChildren(getLastInputType());
	if (state != curState) {
		onStateChanged(curState, state);
	}
	if (state != curState || forceUpdate) {
		curState = state;
		doSetState(state);
		return true;
	}
	return false;
}

void UIClickable::onEnabledChanged()
{
	if (!isEnabled()) {
		for (auto& i : held) {
			i = false;
		}
	}
	doForceUpdate();
}

void UIClickable::onShortcutPressed()
{
}

void UIClickable::onMouseClicked(Vector2f mousePos, int button)
{
	switch (button) {
	case 0:
		onClicked(mousePos);
		break;
	case 1:
		onMiddleClicked(mousePos);
		break;
	case 2:
		onRightClicked(mousePos);
		break;
	default:
		Logger::logError("Unhandled mouse button clicked: " + toString(button));
	}
}

void UIClickable::onMouseDoubleClicked(Vector2f mousePos, int button)
{
	switch (button) {
	case 0:
		onDoubleClicked(mousePos);
		break;
	case 1:
		// do nothing
		break;
	case 2:
		onRightDoubleClicked(mousePos);
		break;
	default:
		Logger::logError("Unhandled mouse button double clicked: " + toString(button));
	}
}

void UIClickable::setMouseExtraBorder(std::optional<Vector4f> override)
{
	mouseExtraBorder = override;
}

void UIClickable::update(Time t, bool)
{
	for (auto& i : clickTime) {
		i += t;
	}
}
