#include "halley/ui/widgets/ui_dropdown.h"
#include "ui_style.h"
#include "widgets/ui_image.h"
#include "widgets/ui_scroll_pane.h"
#include "widgets/ui_scrollbar.h"
#include "widgets/ui_list.h"
#include "halley/text/i18n.h"

using namespace Halley;

UIDropdown::UIDropdown(String id, UIStyle style, UIStyle scrollbarStyle, UIStyle listStyle, const std::vector<LocalisedString>& os, int defaultOption)
	: UIClickable(id, {})
	, style(style)
	, scrollbarStyle(scrollbarStyle)
	, listStyle(listStyle)
	, curOption(defaultOption)
{
	sprite = style.getSprite("normal");

	setOptions(os);
}

void UIDropdown::setSelectedOption(int option)
{
	int nextOption = clamp(option, 0, int(options.size()));
	if (curOption != nextOption) {
		curOption = nextOption;
		label.setText(options.at(curOption));
		sendEvent(UIEvent(UIEventType::DropboxSelectionChanged, getId(), options[curOption].getString(), curOption));
	}
}

int UIDropdown::getSelectedOption() const
{
	return curOption;
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

void UIDropdown::setOptions(const std::vector<LocalisedString>& os)
{
	options = os;
	if (options.empty()) {
		options.emplace_back();
	}
	curOption = clamp(curOption, 0, int(options.size() - 1));
	label = style.getTextRenderer("label").clone().setText(options[curOption]);

	float maxExtents = 0;
	for (auto& o: options) {
		maxExtents = std::max(maxExtents, label.clone().setText(o).getExtents().x);
	}
	setMinSize(Vector2f(maxExtents + 19, 14)); // HACK
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

void UIDropdown::draw(UIPainter& painter) const
{
	painter.draw(sprite);
	painter.draw(label);
}

void UIDropdown::update(Time t, bool moved)
{
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
		label.setAlignment(0.0f).setPosition(getPosition() + Vector2f(3, 0));

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

		scrollPane = std::make_shared<UIScrollPane>(Vector2f(0, 80));
		scrollPane->add(dropdownList);

		auto scrollBar = std::make_shared<UIScrollBar>(UIScrollDirection::Vertical, scrollbarStyle);
		scrollBar->setScrollPane(*scrollPane);

		dropdownWindow = std::make_shared<UIImage>(style.getSprite("background"), UISizer(UISizerType::Horizontal), style.getBorder("innerBorder"));
		dropdownWindow->add(scrollPane, 1);
		dropdownWindow->add(scrollBar);
		dropdownWindow->setMinSize(Vector2f(getSize().x, getSize().y));
		addChild(dropdownWindow);

		dropdownList->getEventHandler().setHandle(UIEventType::ListAccept, [=] (const UIEvent& event)
		{
			setSelectedOption(event.getIntData());
			close();
		});

		forceLayout();
		auto sz = dropdownList->getSize();
		scrollPane->setScrollSpeed(ceil(sz.y / options.size()));
		scrollPane->update(0, false);
	}
}

void UIDropdown::close()
{
	if (isOpen) {
		isOpen = false;

		dropdownWindow->destroy();
		dropdownWindow.reset();
	}
}

void UIDropdown::drawChildren(UIPainter& painter) const
{
	auto p = painter.withAdjustedLayer(1);
	UIWidget::drawChildren(p);
}
