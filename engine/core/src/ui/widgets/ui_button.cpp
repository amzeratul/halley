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
		if (isFocused()) {
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
		held = false;
	}
}

bool UIButton::setState(State state)
{
	if (state != curState) {
		curState = state;

		if (state == State::Up) {
			sprite = style->buttonNormal;
		} else if (state == State::Down) {
			sprite = style->buttonDown;
		} else if (state == State::Hover) {
			sprite = style->buttonHover;
		}

		return true;
	}
	return false;
}
