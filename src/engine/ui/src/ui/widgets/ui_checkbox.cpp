#include "widgets/ui_checkbox.h"
#include "ui_style.h"
#include "halley/ui/ui_data_bind.h"

using namespace Halley;

UICheckbox::UICheckbox(String id, UIStyle style, bool checked)
	: UIClickable(id, {})
	, checked(checked)
{
	styles.emplace_back(std::move(style));
	UICheckbox::doSetState(State::Up);
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
		sendEvent(UIEvent(UIEventType::CheckboxUpdated, getId(), checked));
		notifyDataBind(checked);
	}
}

void UICheckbox::onClicked(Vector2f)
{
	setChecked(!checked);
}

void UICheckbox::doSetState(State state)
{
	const auto& style = styles.at(0);
	if (isEnabled()) {
		if (state == State::Hover || state == State::Down) {
			sprite = checked ? style.getSprite("checkedHover") : style.getSprite("hover");
		} else {
			sprite = checked ? style.getSprite("checked") : style.getSprite("normal");
		}
	} else {
		sprite = checked ? style.getSprite("checkedDisabled") : style.getSprite("disabled");
	}
}

void UICheckbox::onManualControlCycleValue(int delta)
{
	if (delta > 0) {
		setChecked(true);
	} else if (delta < 0) {
		setChecked(false);
	}
}

void UICheckbox::onManualControlActivate()
{
	setChecked(!checked);
}

void UICheckbox::readFromDataBind()
{
	setChecked(getDataBind()->getIntData() != 0);
}
