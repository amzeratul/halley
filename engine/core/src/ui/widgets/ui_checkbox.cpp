#include "ui/widgets/ui_checkbox.h"
#include "ui/ui_style.h"

using namespace Halley;

UICheckbox::UICheckbox(String id, std::shared_ptr<UIStyle> style, bool checked)
	: UIButton(id, style)
	, checked(checked)
{
	sprite = style->checkboxNormal;
	setMinSize(sprite.getScaledSize().abs());
}

bool UICheckbox::isChecked() const
{
	return checked;
}

void UICheckbox::setChecked(bool c)
{
	checked = c;
}

void UICheckbox::onClicked()
{
	checked = !checked;
	sendEvent(UIEvent(UIEventType::CheckboxUpdated, getId(), checked ? "true" : "false"));
}

bool UICheckbox::setState(State state)
{
	if (curState != state) {
		if (state == State::Hover) {
			sprite = checked ? style->checkboxCheckedHover : style->checkboxNormalHover;
		} else {
			sprite = checked ? style->checkboxChecked : style->checkboxNormal;
		}
		curState = state;
		return true;
	}
	return false;
}
