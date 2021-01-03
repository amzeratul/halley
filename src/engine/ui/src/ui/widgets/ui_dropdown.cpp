#include "halley/ui/widgets/ui_dropdown.h"
#include "ui_style.h"
#include "widgets/ui_image.h"
#include "widgets/ui_scroll_pane.h"
#include "widgets/ui_scrollbar.h"
#include "widgets/ui_list.h"
#include "halley/text/i18n.h"
#include "halley/ui/ui_data_bind.h"

using namespace Halley;

UIDropdown::UIDropdown(String id, UIStyle style, std::vector<LocalisedString> os, int defaultOption)
	: UIClickable(std::move(id), Vector2f(style.getFloat("minSize"), style.getFloat("minSize")))
	, style(style)
	, curOption(defaultOption)
{
	sprite = style.getSprite("normal");

	setOptions(std::move(os));
}

void UIDropdown::setSelectedOption(int option)
{
	const int nextOption = clamp(option, 0, static_cast<int>(options.size()) - 1);
	if (curOption != nextOption) {
		curOption = nextOption;
		label.setText(options.at(curOption).label);
		icon = options.at(curOption).icon;
		sendEvent(UIEvent(UIEventType::DropboxSelectionChanged, getId(), options[curOption].id, curOption));

		if (getDataBindFormat() == UIDataBind::Format::String) {
			notifyDataBind(options[curOption].id);
		} else {
			notifyDataBind(curOption);
		}
	}
}

void UIDropdown::setSelectedOption(const String& id)
{
	const auto iter = std::find_if(options.begin(), options.end(), [&] (const auto& o) { return o.id == id; });
	if (iter != options.end()) {
		setSelectedOption(static_cast<int>(iter - options.begin()));
	}
}

int UIDropdown::getSelectedOption() const
{
	return curOption;
}

String UIDropdown::getSelectedOptionId() const
{
	return curOption >= 0 && curOption < static_cast<int>(options.size()) ? options[curOption].id : "";
}

LocalisedString UIDropdown::getSelectedOptionText() const
{
	return options[curOption].label;
}

int UIDropdown::getNumberOptions() const
{
	return static_cast<int>(options.size());
}

void UIDropdown::setInputButtons(const UIInputButtons& buttons)
{
	inputButtons = buttons;
	if (dropdownList) {
		dropdownList->setInputButtons(buttons);
	}
}

void UIDropdown::updateOptionLabels() {
	auto tempLabel = style.getTextRenderer("label");

	label = tempLabel.clone().setText(options[curOption].label);
	icon = options[curOption].icon;

	const float iconGap = style.getFloat("iconGap");
	
	float maxExtents = 0;
	for (auto& o: options) {
		const float iconSize = o.icon.hasMaterial() ? o.icon.getScaledSize().x + iconGap : 0;
		const float strSize = tempLabel.setText(o.label).getExtents().x;
		maxExtents = std::max(maxExtents, iconSize + strSize);
	}

	const auto minSizeMargins = style.getBorder("minSizeMargins");
	const auto minSize = Vector2f(maxExtents, 0) + minSizeMargins.xy();
	setMinSize(Vector2f::max(getMinimumSize(), minSize));
}

void UIDropdown::setOptions(std::vector<LocalisedString> os, int defaultOption)
{
	setOptions({}, std::move(os), defaultOption);
}

void UIDropdown::setOptions(std::vector<String> optionIds, int defaultOption)
{
	setOptions(std::move(optionIds), {}, defaultOption);
}

void UIDropdown::setOptions(const I18N& i18n, const String& i18nPrefix, std::vector<String> optionIds, int defaultOption)
{
	setOptions(optionIds, i18n.getVector(i18nPrefix, optionIds), defaultOption);
}

void UIDropdown::setOptions(std::vector<String> oIds, std::vector<LocalisedString> os, int defaultOption)
{
	std::vector<Entry> entries;
	entries.resize(std::max(os.size(), oIds.size()));
	for (size_t i = 0; i < entries.size(); ++i) {
		// Be careful, do label first as id will get moved out
		entries[i].label = os.size() > i ? std::move(os[i]) : LocalisedString::fromUserString(oIds[i]);
		entries[i].id = oIds.size() > i ? std::move(oIds[i]) : toString(i);
	}

	setOptions(std::move(entries), defaultOption);
}

void UIDropdown::setOptions(std::vector<Entry> os, int defaultOption)
{
	options = std::move(os);

	if (options.empty()) {
		options.emplace_back();
	}
	curOption = clamp(curOption, 0, static_cast<int>(options.size() - 1));
	updateOptionLabels();

	if (defaultOption != -1) {
		setSelectedOption(defaultOption);
	}
}

void UIDropdown::onManualControlCycleValue(int delta)
{
	setSelectedOption(modulo(curOption + delta, int(options.size())));
}

void UIDropdown::onManualControlActivate()
{
	focus();
	open();
}

bool UIDropdown::canReceiveFocus() const
{
	return true;
}

void UIDropdown::draw(UIPainter& painter) const
{
	painter.draw(sprite);
	if (icon.hasMaterial()) {
		painter.draw(icon);
	}
	painter.draw(label);
}

void UIDropdown::update(Time t, bool moved)
{
	bool optionsUpdated = false;
	for (auto& o: options) {
		if (o.label.checkForUpdates()) {
			optionsUpdated = true;
		}
	}
	if (optionsUpdated) {
		updateOptionLabels();
	}

	if (isOpen) {
		auto focus = getRoot()->getCurrentFocus();
		if (!focus || (focus != this && !focus->isDescendentOf(*this))) {
			close();
		}
	}

	bool needUpdate = true;
	sprite = isEnabled() ? (isOpen ? style.getSprite("open") : (isMouseOver() ? style.getSprite("hover") : style.getSprite("normal"))) : style.getSprite("disabled");

	if (needUpdate) {
		sprite.setPos(getPosition()).scaleTo(getSize());

		const Vector2f basePos = getPosition() + style.getBorder("labelBorder").xy();
		Vector2f iconOffset;
		if (icon.hasMaterial()) {
			icon.setPosition(basePos);
			iconOffset = Vector2f(style.getFloat("iconGap") + icon.getScaledSize().x, 0.0f);
		}
		label.setAlignment(0.0f).setPosition(basePos + iconOffset);

		if (dropdownWindow) {
			dropdownWindow->setPosition(getPosition() + Vector2f(0.0f, getSize().y));
		}
	}
}

void UIDropdown::onClicked(Vector2f mousePos)
{
	if (isOpen) {
		close();
	} else {
		open();
	}
}

void UIDropdown::doSetState(State state)
{
}

bool UIDropdown::isFocusLocked() const
{
	return isOpen || UIClickable::isFocusLocked();
}

void UIDropdown::readFromDataBind()
{
	auto data = getDataBind();
	if (data->getFormat() == UIDataBind::Format::String) {
		setSelectedOption(data->getStringData());
	} else {
		setSelectedOption(data->getIntData());
	}
}

void UIDropdown::open()
{
	if (!isOpen) {
		isOpen = true;

		const float iconGap = style.getFloat("iconGap");
	
		dropdownList = std::make_shared<UIList>(getId() + "_list", style.getSubStyle("listStyle"));
		int i = 0;
		for (const auto& o: options) {
			if (o.icon.hasMaterial()) {
				auto item = std::make_shared<UISizer>(UISizerType::Horizontal, iconGap);
				item->add(std::make_shared<UIImage>(o.icon));
				item->add(dropdownList->makeLabel(toString(i++) + "_label", o.label));
				dropdownList->addItem(toString(i++), std::move(item));
			} else {
				dropdownList->addTextItem(toString(i++), o.label);
			}
		}
		dropdownList->setSelectedOption(curOption);
		dropdownList->setInputButtons(inputButtons);
		getRoot()->setFocus(dropdownList);

		const auto standardHeight = style.getFloat("height");
		const auto distanceFromBottom = getRoot()->getRect().getBottom() - getRect().getBottom() - 5.0f;
		const auto height = distanceFromBottom < 0 ? standardHeight : std::min(standardHeight, distanceFromBottom);

		scrollPane = std::make_shared<UIScrollPane>(getId() + "_pane", Vector2f(0, height), UISizer(UISizerType::Vertical, 0));
		scrollPane->add(dropdownList);

		auto scrollBar = std::make_shared<UIScrollBar>(getId() + "_vbar", UIScrollDirection::Vertical, style.getSubStyle("scrollbarStyle"));
		scrollBar->setScrollPane(*scrollPane);

		dropdownWindow = std::make_shared<UIImage>(style.getSprite("background"), UISizer(UISizerType::Horizontal), style.getBorder("innerBorder"));
		dropdownWindow->add(scrollPane, 1);
		dropdownWindow->add(scrollBar);
		dropdownWindow->setMinSize(Vector2f(getSize().x, getSize().y));
		addChild(dropdownWindow);

		dropdownList->setHandle(UIEventType::ListAccept, [=] (const UIEvent& event)
		{
			setSelectedOption(event.getIntData());
			close();
		});

		dropdownList->setHandle(UIEventType::ListCancel, [=] (const UIEvent& event)
		{
			close();
		});

		sendEvent(UIEvent(UIEventType::DropdownOpened, getId(), getSelectedOptionId(), curOption));

		forceLayout();
		auto sz = dropdownList->getSize();
		scrollPane->setScrollSpeed(ceil(2 * sz.y / options.size()));
		scrollPane->update(0, false);

		playSound(style.getString("openSound"));
	}
}

void UIDropdown::close()
{
	if (isOpen) {
		isOpen = false;

		scrollPane->destroy();
		scrollPane.reset();
		dropdownList->destroy();
		dropdownList.reset();
		dropdownWindow->destroy();
		dropdownWindow.reset();

		sendEvent(UIEvent(UIEventType::DropdownClosed, getId(), getSelectedOptionId(), curOption));
		playSound(style.getString("closeSound"));
	}
}

void UIDropdown::drawChildren(UIPainter& painter) const
{
	auto p = painter.withAdjustedLayer(1);
	UIWidget::drawChildren(p);
}
