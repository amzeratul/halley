#include "ui/widgets/ui_button.h"
#include "ui/ui_style.h"

using namespace Halley;

UIButton::UIButton(String id, std::shared_ptr<UIStyle> s)
	: UIWidget(id, {}, UISizer(UISizerType::Horizontal, 1), s->buttonInnerBorder)
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
	bool dirty = moved;

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

	if (dirty) {
		sprite.scaleTo(getSize()).setPos(getPosition());
	}
}

bool UIButton::isFocusable() const
{
	return true;
}

bool UIButton::isFocusLocked() const
{
	return held;
}

void UIButton::pressMouse(int button)
{
	if (button == 0) {
		held = true;
	}
}

void UIButton::releaseMouse(int button)
{
	if (button == 0) {
		if (held && isMouseOver()) {
			sendEvent(UIEvent(UIEventType::ButtonClicked, getId()));
		}
		held = false;
	}
}

void UIButton::onClick(UIEventCallback callback)
{
	if (!getEventHandler()) {
		createEventHandler();
	}

	getEventHandler()->setHandle(UIEventType::ButtonClicked, callback);
}

void UIButton::playSound(const std::shared_ptr<const AudioClip>& clip)
{
	if (clip) {
		getRoot().playSound(clip);
	}
}

bool UIButton::setState(State state)
{
	if (state != curState) {
		curState = state;

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

		return true;
	}
	return false;
}
