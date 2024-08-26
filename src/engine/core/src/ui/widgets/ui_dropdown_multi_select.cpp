#include "halley/ui/widgets/ui_dropdown_multi_select.h"
#include "halley/ui/ui_style.h"

using namespace Halley;

UIDropdownMultiSelect::UIDropdownMultiSelect(String id, UIStyle style, Vector<LocalisedString> os, Vector<int> defaultOptions)
	: UIDropdown(std::move(id), std::move(style), std::move(os), 0)
	, curOptions(std::move(defaultOptions))
{
}

void UIDropdownMultiSelect::setSelectedOption(int option)
{
	UIDropdown::setSelectedOption(option);	
	toggleOption(option);
}

// Needed for C++ overload resolution
void UIDropdownMultiSelect::setSelectedOption(const String& id)
{
	UIDropdown::setSelectedOption(id);
}

void UIDropdownMultiSelect::setOptions(Vector<Entry> options, Vector<int> defaultOptions)
{
	const auto emptyIcon = styles[0].getSprite("notSelectedIcon");

	for (auto& option: options) {
		option.icon = emptyIcon;
	}

	curOptions = std::move(defaultOptions);
	UIDropdown::setOptions(std::move(options), -1);
	updateOptionLabels();
}

void UIDropdownMultiSelect::readFromDataBind()
{
	auto data = getDataBind();
	curOptions = {};

	if (data->getFormat() == UIDataBind::Format::String) {
		const auto& id = data->getStringData();
		const auto iter = std::find_if(options.begin(), options.end(), [&] (const auto& o) { return o.id == id; });
		if (iter != options.end()) {
			const auto idx = static_cast<int>(iter - options.begin());
			curOptions.push_back(idx);
		}
	} else {
		auto value = data->getIntData();
		for (int i = 0; i < static_cast<int>(options.size()); ++i) {
			auto& option = options[i];
			const int curMask = option.value;
			if (value & curMask) {
				curOptions.push_back(i);
			}
		}
	}

	updateOptionLabels();
}

void UIDropdownMultiSelect::toggleOption(int idx)
{
	const auto iter = std::find(curOptions.begin(), curOptions.end(), idx);
	if (iter != curOptions.end()) {
		curOptions.erase(iter);
	} else {
		curOptions.push_back(idx);
	}
	updateOptionLabels();
	notify();
}

void UIDropdownMultiSelect::updateTopLabel()
{
	const auto& style = styles.at(0);
	label = style.getTextRenderer("label");

	LocalisedString labelText;
	if (curOptions.empty()) {
		labelText = LocalisedString::fromUserString("empty");
	} else {
		for (auto i = 0; i < int(curOptions.size()); i++) {
			const auto idx = curOptions[i];
			labelText += LocalisedString::fromUserString(options.at(idx).id);
			if (i != int(curOptions.size()) - 1) {
				labelText += LocalisedString::fromUserString(", ");
			}
		}
	}
	label.setText(labelText);
}

void UIDropdownMultiSelect::updateOptionLabels()
{
	const auto emptyIcon = styles[0].getSprite("notSelectedIcon");
	const auto selIcon = styles[0].getSprite("selectedIcon");

	for (auto i = 0; i < int(options.size()); i++) {
		const auto isSelected = std::find(curOptions.begin(), curOptions.end(), i) != curOptions.end();
		options[i].icon = isSelected ? selIcon : emptyIcon;
	}

	UIDropdown::updateOptionLabels();
}

bool UIDropdownMultiSelect::canNotifyAsDropdown() const
{
	return false;
}

void UIDropdownMultiSelect::notify()
{
	int value = 0;
	for (int cur: curOptions) {
		value |= options.at(cur).value;
	}
	notifyDataBind(value);
}
