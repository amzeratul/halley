#include "ui/widgets/ui_button.h"
#include "ui/ui_style.h"
#include "ui/ui_painter.h"

using namespace Halley;

UIButton::UIButton(String id, std::shared_ptr<UIStyle> s, Maybe<UISizer> sizer, Vector4f innerBorder)
	: UIClickable(id, {}, sizer, innerBorder)
	, style(s)
{
	sprite = style->getSprite("button.normal");
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
		sprite = style->getSprite("button.borderOnly");
	} else {
		if (!isEnabled()) {
			sprite = style->getSprite("button.disabled");
		} else if (state == State::Up) {
			sprite = style->getSprite("button.normal");
			playSound(style->getAudioClip("button.upSound"));
		} else if (state == State::Down) {
			sprite = style->getSprite("button.down");
			playSound(style->getAudioClip("button.downSound"));
		} else if (state == State::Hover) {
			sprite = style->getSprite("button.hover");
			playSound(style->getAudioClip("button.hoverSound"));
		}
	}
}
