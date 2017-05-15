#include "ui/widgets/ui_checkbox.h"
#include "ui/ui_style.h"

using namespace Halley;

UICheckbox::UICheckbox(String id, std::shared_ptr<UIStyle> style, bool checked)
	: UIClickable(id, {})
	, style(style)
	, checked(checked)
{
	sprite = style->checkboxNormal;
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
	checked = c;
}

void UICheckbox::onClicked(Vector2f)
{
	checked = !checked;
	sendEvent(UIEvent(UIEventType::CheckboxUpdated, getId(), checked ? "true" : "false"));
}

void UICheckbox::doSetState(State state)
{
	if (isEnabled()) {
		if (state == State::Hover) {
			sprite = checked ? style->checkboxCheckedHover : style->checkboxNormalHover;
		} else {
			sprite = checked ? style->checkboxChecked : style->checkboxNormal;
		}
	} else {
		sprite = checked ? style->checkboxCheckedDisabled : style->checkboxDisabled;
	}
}
