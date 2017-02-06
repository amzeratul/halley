#include "ui/widgets/ui_button.h"
#include "ui/ui_style.h"

using namespace Halley;

UIClickable::UIClickable(String id, Vector2f minSize, Maybe<UISizer> sizer, Vector4f innerBorder)
	: UIWidget(id, minSize, sizer, innerBorder)
{
}

UIButton::UIButton(String id, std::shared_ptr<UIStyle> s)
	: UIClickable(id, {}, UISizer(UISizerType::Horizontal, 1), s->buttonInnerBorder)
	, style(s)
{
	sprite = style->buttonNormal;
}

void UIButton::draw(UIPainter& painter) const
{
	painter.draw(sprite);
}

void UIButton::update(Time t, bool moved)
{
	bool dirty = updateButton() | moved;
	if (dirty) {
		sprite.scaleTo(getSize()).setPos(getPosition());
	}
}

bool UIClickable::isFocusable() const
{
	return true;
}

bool UIClickable::isFocusLocked() const
{
	return held;
}

void UIClickable::pressMouse(int button)
{
	if (button == 0) {
		held = true;
	}
}

void UIClickable::releaseMouse(int button)
{
	if (button == 0) {
		if (held && isMouseOver()) {
			onClicked();
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
	bool dirty = forceUpdate;
	forceUpdate = false;
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
	return dirty;
}

void UIClickable::doForceUpdate()
{
	forceUpdate = true;
}

void UIButton::onClicked()
{
	sendEvent(UIEvent(UIEventType::ButtonClicked, getId()));
}

void UIButton::setInputType(UIInputType uiInput)
{
	if (uiInput != curInputType) {
		curInputType = uiInput;
		borderOnly = getOnlyEnabledWithInput() != UIInputType::Undefined && curInputType != UIInputType::Mouse;
		doSetState(getCurState());
		doForceUpdate();
	}
}

bool UIButton::isFocusable() const
{
	return borderOnly ? false : UIClickable::isFocusable();
}

bool UIButton::isFocusLocked() const
{
	return borderOnly ? false : UIClickable::isFocusLocked();
}

void UIButton::doSetState(State state)
{
	if (borderOnly) {
		sprite = style->buttonBorderOnly;
	} else {
		if (state == State::Up) {
			sprite = style->buttonNormal;
			playSound(style->buttonUpSound);
		} else if (state == State::Down) {
			sprite = style->buttonDown;
			playSound(style->buttonDownSound);
		} else if (state == State::Hover) {
			sprite = style->buttonHover;
			playSound(style->buttonHoverSound);
		}
	}
}

bool UIClickable::setState(State state)
{
	if (state != curState) {
		curState = state;
		doSetState(state);
		return true;
	}
	return false;
}
