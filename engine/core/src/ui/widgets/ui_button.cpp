#include "ui/widgets/ui_button.h"
#include "ui/ui_style.h"
#include "ui/ui_painter.h"

using namespace Halley;

UIButton::UIButton(String id, std::shared_ptr<UIStyle> s, Maybe<UISizer> sizer, Vector4f innerBorder)
	: UIClickable(id, {}, sizer, innerBorder)
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

void UIButton::onClicked(Vector2f)
{
	sendEvent(UIEvent(UIEventType::ButtonClicked, getId()));
}

void UIButton::setInputType(UIInputType uiInput)
{
	if (uiInput != curInputType) {
		curInputType = uiInput;
		borderOnly = !getOnlyEnabledWithInput().empty() && curInputType != UIInputType::Mouse && curInputType != UIInputType::Keyboard;
		doForceUpdate();
		setState(State::Up);
		doSetState(State::Up);
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
		if (!isEnabled()) {
			sprite = style->buttonDisabled;
		} else if (state == State::Up) {
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
