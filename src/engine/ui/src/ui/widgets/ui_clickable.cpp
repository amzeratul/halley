#include "widgets/ui_clickable.h"
using namespace Halley;

UIClickable::UIClickable(String id, Vector2f minSize, Maybe<UISizer> sizer, Vector4f innerBorder)
	: UIWidget(id, minSize, std::move(sizer), innerBorder)
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
		}
		held = false;
	}
}

void UIClickable::onClick(UIEventCallback callback)
{
	getEventHandler().setHandle(UIEventType::ButtonClicked, callback);
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

void UIClickable::onInput(const UIInputResults& input)
{
	if (input.isButtonPressed(UIInput::Button::Accept)) {
		onClicked(Vector2f());
	}
}

bool UIClickable::setState(State state)
{
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
