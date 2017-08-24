#include "ui/widgets/ui_checkbox.h"
#include "ui/ui_style.h"

using namespace Halley;

UICheckbox::UICheckbox(String id, UIStyle style, bool checked)
	: UIClickable(id, {})
	, style(style)
	, checked(checked)
{
	sprite = style.getSprite("normal");
	setMinSize(sprite.getScaledSize().abs());
}

void UICheckbox::draw(UIPainter& painter) const
{
	painter.draw(sprite);
}

void UICheckbox::update(Time t, bool moved)
{
	bool dirty = updateButton() | moved;
	if (dirty) {
		sprite.scaleTo(getSize()).setPos(getPosition());
	}
}

bool UICheckbox::isChecked() const
{
	return checked;
}

void UICheckbox::setChecked(bool c)
{
	if (checked != c) {
		checked = c;
		doForceUpdate();
	}
}

void UICheckbox::onClicked(Vector2f)
{
	checked = !checked;
	sendEvent(UIEvent(UIEventType::CheckboxUpdated, getId(), checked ? "true" : "false", checked ? 1 : 0));
}

void UICheckbox::doSetState(State state)
{
	if (isEnabled()) {
		if (state == State::Hover) {
			sprite = checked ? style.getSprite("checkedHover") : style.getSprite("hover");
		} else {
			sprite = checked ? style.getSprite("checked") : style.getSprite("normal");
		}
	} else {
		sprite = checked ? style.getSprite("checkedDisabled") : style.getSprite("disabled");
	}
}
