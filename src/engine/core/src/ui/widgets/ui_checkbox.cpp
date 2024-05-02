#include "halley/ui/widgets/ui_checkbox.h"
#include "halley/ui/ui_style.h"
#include "halley/ui/ui_data_bind.h"
#include "halley/ui/widgets/ui_image.h"

using namespace Halley;

UICheckbox::UICheckbox(String id, UIStyle style, bool checked, LocalisedString labelText)
	: UIClickable(id, {}, UISizer(UISizerType::Horizontal, style.getFloat("gap")))
	, checked(checked)
{
	label = std::make_shared<UILabel>(id + "_label", style, std::move(labelText));
	label->setActive(!label->getText().getString().isEmpty());
	image = std::make_shared<UIImage>(id + "_image", checked ? style.getSprite("checked") : style.getSprite("normal"));

	UIWidget::add(image, 0);
	UIWidget::add(label, 1, {}, UISizerAlignFlags::CentreVertical | UISizerAlignFlags::Left);

	styles.emplace_back(std::move(style));
	UICheckbox::doSetState(State::Up);
}

void UICheckbox::update(Time t, bool moved)
{
	updateButton();
	UIClickable::update(t, moved);
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

void UICheckbox::onClicked(Vector2f, KeyMods)
{
	setChecked(!checked);
}

void UICheckbox::doSetState(State state)
{
	const auto& style = styles.at(0);
	if (isEnabled()) {
		if (state == State::Hover || state == State::Down) {
			image->setSprite(checked ? style.getSprite("checkedHover") : style.getSprite("hover"));
		} else {
			image->setSprite(checked ? style.getSprite("checked") : style.getSprite("normal"));
		}
	} else {
		image->setSprite(checked ? style.getSprite("checkedDisabled") : style.getSprite("disabled"));
	}
}

void UICheckbox::onManualControlCycleValue(int delta)
{
	if (isEnabled()) {
		if (delta > 0) {
			setChecked(true);
		} else if (delta < 0) {
			setChecked(false);
		}
	}
}

void UICheckbox::onManualControlActivate()
{
	if (isEnabled()) {
		setChecked(!checked);
	}
}

void UICheckbox::readFromDataBind()
{
	setChecked(getDataBind()->getIntData() != 0);
}

void UICheckbox::setLabel(LocalisedString str)
{
	label->setActive(!str.getString().isEmpty());
	label->setText(std::move(str));
}
