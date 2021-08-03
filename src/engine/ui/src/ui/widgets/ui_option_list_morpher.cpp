#include "halley/ui/widgets/ui_option_list_morpher.h"
#include "ui_style.h"
#include "widgets/ui_image.h"
#include "halley/text/i18n.h"
#include "halley/ui/ui_data_bind.h"
#include "halley/support/logger.h"
#include "widgets/ui_dropdown.h"
#include "widgets/ui_spin_list.h"

using namespace Halley;

UIOptionListMorpher::UIOptionListMorpher(String id, UIStyle dropdownStyle, UIStyle spinlistStyle, std::vector<LocalisedString> os, int defaultOption)
	: UIWidget(std::move(id), {})
	, dropdownStyle(dropdownStyle)
	, spinlistStyle(spinlistStyle)
{
	styles.emplace_back(dropdownStyle);
	styles.emplace_back(spinlistStyle);
	
	setOptions(std::move(os), defaultOption);
}

void UIOptionListMorpher::setSelectedOption(int option)
{
	dropdown->setSelectedOption(option);
	spinlist->setSelectedOption(option);
}

void UIOptionListMorpher::setSelectedOption(const String& id)
{
	dropdown->setSelectedOption(id);
	spinlist->setSelectedOption(id);
}

int UIOptionListMorpher::getSelectedOption() const
{
	if (currentMode == OptionListMorpherMode::Dropdown) {
		return dropdown->getSelectedOption();
	}
	return spinlist->getSelectedOption();
}

String UIOptionListMorpher::getSelectedOptionId() const
{
	if (currentMode == OptionListMorpherMode::Dropdown) {
		return dropdown->getSelectedOptionId();
	}
	return spinlist->getSelectedOptionId();
}

LocalisedString UIOptionListMorpher::getSelectedOptionText() const
{
	if (currentMode == OptionListMorpherMode::Dropdown) {
		return dropdown->getSelectedOptionText();
	}
	return spinlist->getSelectedOptionText();
}

void UIOptionListMorpher::setInputButtons(const UIInputButtons& buttons)
{
	dropdown->setInputButtons(buttons);
	spinlist->setInputButtons(buttons);
}

void UIOptionListMorpher::setInputType(UIInputType uiInput) {
	if (lastInputType != uiInput) {
		if (uiInput == UIInputType::Mouse || uiInput == UIInputType::Keyboard) {
			dropdown->setSelectedOption(spinlist->getSelectedOption());
		}
		else if (uiInput == UIInputType::Gamepad) {
			spinlist->setSelectedOption(dropdown->getSelectedOption());
		}
	}

	if (uiInput == UIInputType::Mouse || uiInput == UIInputType::Keyboard) {
		currentMode = OptionListMorpherMode::Dropdown;
	}
	else if (uiInput == UIInputType::Gamepad) {
		currentMode = OptionListMorpherMode::SpinList;
	}

	lastInputType = uiInput;
}

void UIOptionListMorpher::setOptions(const std::vector<LocalisedString> os, int defaultOption)
{
	setOptions({}, os, defaultOption);
}

void UIOptionListMorpher::setOptions(std::vector<String> oIds, const std::vector<LocalisedString>& os, int defaultOption)
{
	makeChildren(os, defaultOption);

	dropdown->setOptions(oIds, os, defaultOption);
	spinlist->setOptions(oIds, os, defaultOption);
}

void UIOptionListMorpher::setOptions(const I18N& i18n, const String& i18nPrefix, const std::vector<String>& optionIds, int defaultOption)
{
	dropdown->setOptions(optionIds, i18n.getVector(i18nPrefix, optionIds), defaultOption);
	spinlist->setOptions(optionIds, i18n.getVector(i18nPrefix, optionIds), defaultOption);
}

void UIOptionListMorpher::onManualControlCycleValue(int delta)
{
	dropdown->onManualControlCycleValue(delta);
	spinlist->onManualControlCycleValue(delta);
}

void UIOptionListMorpher::onManualControlActivate()
{
	focus();
}

bool UIOptionListMorpher::canReceiveFocus() const
{
	return true;
}

void UIOptionListMorpher::draw(UIPainter& painter) const
{

}

void UIOptionListMorpher::update(Time t, bool moved)
{
	dropdown->setPosition(getPosition());
	spinlist->setPosition(getPosition());

	auto listMinSize = std::max(dropdown->getMinimumSize(), spinlist->getMinimumSize());
	setMinSize(std::max(getMinimumSize(), listMinSize));
	dropdown->setMinSize(getSize());
	spinlist->setMinSize(getSize());

	dropdown->setEnabled(isEnabled());
	spinlist->setEnabled(isEnabled());
}

void UIOptionListMorpher::readFromDataBind()
{
	auto data = getDataBind();
	if (data->getFormat() == UIDataBind::Format::String) {
		setSelectedOption(data->getStringData());
	}
	else {
		setSelectedOption(data->getIntData());
	}
}

void UIOptionListMorpher::makeChildren(const std::vector<LocalisedString>& os, int defaultOption) {
	if (dropdown) {
		return;
	}

	dropdown = std::make_shared<UIDropdown>(getId(), dropdownStyle, os, defaultOption);
	dropdown->setOnlyEnabledWithInputs({ { UIInputType::Mouse, UIInputType::Keyboard } });
	add(dropdown, 1, {}, UISizerFillFlags::Fill);

	spinlist = std::make_shared<UISpinList>(getId(), spinlistStyle, os, defaultOption);
	spinlist->setOnlyEnabledWithInputs({ UIInputType::Gamepad });
	add(spinlist, 1, {}, UISizerFillFlags::Fill);
}

void UIOptionListMorpher::drawChildren(UIPainter& painter) const
{
	auto p = painter.withAdjustedLayer(1);
	UIWidget::drawChildren(p);
}
