#include "halley/ui/widgets/ui_dropdown.h"
#include "ui_style.h"
#include "widgets/ui_image.h"
#include "widgets/ui_scroll_pane.h"
#include "widgets/ui_scrollbar.h"
#include "widgets/ui_list.h"
#include "halley/text/i18n.h"
#include "halley/ui/ui_data_bind.h"

using namespace Halley;

UIDropdown::UIDropdown(String id, UIStyle style, UIStyle scrollbarStyle, UIStyle listStyle, std::vector<LocalisedString> os, int defaultOption)
	: UIClickable(std::move(id), Vector2f(style.getFloat("minSize"), style.getFloat("minSize")))
	, style(style)
	, scrollbarStyle(std::move(scrollbarStyle))
	, listStyle(std::move(listStyle))
	, curOption(defaultOption)
{
	sprite = style.getSprite("normal");

	setOptions(std::move(os));
}

void UIDropdown::setSelectedOption(int option)
{
	int nextOption = clamp(option, 0, int(options.size()) - 1);
	if (curOption != nextOption) {
		curOption = nextOption;
		label.setText(options.at(curOption));
		sendEvent(UIEvent(UIEventType::DropboxSelectionChanged, getId(), optionIds[curOption], curOption));

		if (getDataBindFormat() == UIDataBind::Format::String) {
			notifyDataBind(optionIds[curOption]);
		} else {
			notifyDataBind(curOption);
		}
	}
}

void UIDropdown::setSelectedOption(const String& id)
{
	auto iter = std::find(optionIds.begin(), optionIds.end(), id);
	if (iter != optionIds.end()) {
		setSelectedOption(int(iter - optionIds.begin()));
	}
}

int UIDropdown::getSelectedOption() const
{
	return curOption;
}

String UIDropdown::getSelectedOptionId() const
{
	return curOption >= 0 && curOption < int(optionIds.size()) ? optionIds[curOption] : "";
}

LocalisedString UIDropdown::getSelectedOptionText() const
{
	return options[curOption];
}

void UIDropdown::setInputButtons(const UIInputButtons& buttons)
{
	inputButtons = buttons;
	if (dropdownList) {
		dropdownList->setInputButtons(buttons);
	}
}

void UIDropdown::setOptions(std::vector<LocalisedString> os, int defaultOption)
{
	setOptions({}, std::move(os), defaultOption);
}

void UIDropdown::setOptions(std::vector<String> optionIds, int defaultOption)
{
	setOptions(std::move(optionIds), {}, defaultOption);
}

void UIDropdown::updateOptionLabels() {
	label = style.getTextRenderer("label").clone().setText(options[curOption]);

	float maxExtents = 0;
	for (auto& o: options) {
		maxExtents = std::max(maxExtents, label.clone().setText(o).getExtents().x);
	}

	auto minSizeMargins = style.getBorder("minSizeMargins");
	auto minSize = Vector2f(maxExtents, 0) + minSizeMargins.xy();
	setMinSize(Vector2f::max(getMinimumSize(), minSize));
}

void UIDropdown::setOptions(std::vector<String> oIds, std::vector<LocalisedString> os, int defaultOption)
{
	if (oIds.empty() && !os.empty()) {
		oIds.resize(os.size());
		for (size_t i = 0; i < oIds.size(); ++i) {
			oIds[i] = toString(i);
		}
	}

	if (os.empty() && !oIds.empty()) {
		os.resize(oIds.size());
		for (size_t i = 0; i < os.size(); ++i) {
			os[i] = LocalisedString::fromUserString(oIds[i]);
		}
	}

	if (oIds.size() != os.size()) {
		throw Exception("Size mismatch between options and option ids", HalleyExceptions::UI);
	}

	options = os;
	optionIds = oIds;

	if (options.empty()) {
		options.emplace_back();
	}
	curOption = clamp(curOption, 0, int(options.size() - 1));
	updateOptionLabels();

	if (defaultOption != -1) {
		setSelectedOption(defaultOption);
	}
}

void UIDropdown::setOptions(const I18N& i18n, const String& i18nPrefix, std::vector<String> optionIds, int defaultOption)
{
	setOptions(optionIds, i18n.getVector(i18nPrefix, optionIds), defaultOption);
}

void UIDropdown::onManualControlCycleValue(int delta)
{
	setSelectedOption(modulo(curOption + delta, int(options.size())));
}

void UIDropdown::onManualControlActivate()
{
	getRoot()->setFocus(shared_from_this());
	open();
}

bool UIDropdown::canReceiveFocus() const
{
	return true;
}

void UIDropdown::draw(UIPainter& painter) const
{
	painter.draw(sprite);
	painter.draw(label);
}

void UIDropdown::update(Time t, bool moved)
{
	bool optionsUpdated = false;
	for (auto& o: options) {
		if (o.checkForUpdates()) {
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
		label.setAlignment(0.0f).setPosition(getPosition() + style.getBorder("labelBorder").xy());

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
	
		dropdownList = std::make_shared<UIList>(getId() + "_list", listStyle);
		int i = 0;
		for (auto& o: options) {
			dropdownList->addTextItem(toString(i++), o);
		}
		dropdownList->setSelectedOption(curOption);
		dropdownList->setInputButtons(inputButtons);
		dropdownList->setFocused(true);

		auto standardHeight = style.getFloat("height");
		auto distanceFromBottom = getRoot()->getRect().getBottom() - getRect().getBottom() - 5.0f;
		auto height = distanceFromBottom < 0 ? standardHeight : std::min(standardHeight, distanceFromBottom);

		scrollPane = std::make_shared<UIScrollPane>(getId() + "_pane", Vector2f(0, height), UISizer(UISizerType::Vertical, 0));
		scrollPane->add(dropdownList);

		auto scrollBar = std::make_shared<UIScrollBar>(getId() + "_vbar", UIScrollDirection::Vertical, scrollbarStyle);
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
		scrollPane->setScrollSpeed(ceil(sz.y / options.size()));
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
