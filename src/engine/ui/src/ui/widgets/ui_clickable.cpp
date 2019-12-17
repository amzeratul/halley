#include "widgets/ui_clickable.h"
using namespace Halley;

UIClickable::UIClickable(String id, Vector2f minSize, Maybe<UISizer> sizer, Vector4f innerBorder)
	: UIWidget(std::move(id), minSize, std::move(sizer), innerBorder)
{
}

bool UIClickable::canInteractWithMouse() const
{
	return true;
}

bool UIClickable::isFocusLocked() const
{
	return held;
}

void UIClickable::pressMouse(Vector2f, int button)
{
	if (button == 0 && isEnabled()) {
		held = true;
	}
}

void UIClickable::releaseMouse(Vector2f mousePos, int button)
{
	if (button == 0 && isEnabled()) {
		if (held && isMouseOver()) {
			onClicked(mousePos);

			if (clickTime < 0.5 && (mousePos - clickPos).length() < 3.0f) {
				onDoubleClicked(mousePos);
			}

			clickTime = 0;
			clickPos = mousePos;
		}
		held = false;
	}
}

void UIClickable::onClick(UIEventCallback callback)
{
	setHandle(UIEventType::ButtonClicked, callback);
}

UIClickable::State UIClickable::getCurState() const
{
	return curState;
}

bool UIClickable::updateButton()
{
	bool dirty = false;
	if (held) {
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

void UIClickable::onDoubleClicked(Vector2f mousePos)
{
}

void UIClickable::onInput(const UIInputResults& input, Time time)
{
	if (input.isButtonPressed(UIInput::Button::Accept)) {
		onShortcutPressed();
		onClicked(Vector2f());
	}
}

Rect4f UIClickable::getMouseRect() const
{
	auto rect = UIWidget::getMouseRect();

	if (mouseExtraBorder) {
		auto& b = mouseExtraBorder.get();
		return Rect4f(rect.getTopLeft() - Vector2f(b.x, b.y), rect.getBottomRight() + Vector2f(b.z, b.w));
	}

	return rect;
}

void UIClickable::onStateChanged(State prev, State next)
{
}

bool UIClickable::setState(State state)
{
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
		held = false;
	}
	doForceUpdate();
}

void UIClickable::onShortcutPressed()
{
}

void UIClickable::setMouseExtraBorder(Maybe<Vector4f> override)
{
	mouseExtraBorder = override;
}

void UIClickable::update(Time t, bool)
{
	clickTime += t;
}
