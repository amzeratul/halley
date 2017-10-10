#include "widgets/ui_button.h"
#include "ui_style.h"
#include "ui_painter.h"

using namespace Halley;

UIButton::UIButton(String id, UIStyle s, Maybe<UISizer> sizer, Maybe<Vector4f> innerBorder)
	: UIClickable(id, s.getSprite("normal").getScaledSize(), sizer, innerBorder ? innerBorder.get() : s.getBorder("innerBorder"))
	, style(s)
{
	sprite = style.getSprite("normal");
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
	lastInputType = uiInput;

	if (uiInput != curInputType) {
		curInputType = uiInput;
		borderOnly = !getOnlyEnabledWithInput().empty() && curInputType != UIInputType::Mouse && curInputType != UIInputType::Keyboard;
		doForceUpdate();
		setState(State::Up);
		doSetState(State::Up);
	}
}

bool UIButton::canInteractWithMouse() const
{
	return borderOnly ? false : UIClickable::canInteractWithMouse();
}

bool UIButton::isFocusLocked() const
{
	return borderOnly ? false : UIClickable::isFocusLocked();
}

void UIButton::doSetState(State state)
{
	if (borderOnly) {
		sprite = style.getSprite("borderOnly");
	} else {
		if (!isEnabled()) {
			sprite = style.getSprite("disabled");
		} else if (state == State::Up) {
			sprite = style.getSprite("normal");
			playSound(style.getAudioClip("upSound"));
		} else if (state == State::Down) {
			sprite = style.getSprite("down");
			playSound(style.getAudioClip("downSound"));
		} else if (state == State::Hover) {
			sprite = style.getSprite("hover");
			playSound(style.getAudioClip("hoverSound"));
		}
	}
}
