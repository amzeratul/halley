#include "widgets/ui_popup_menu.h"

#include "widgets/ui_image.h"
#include "widgets/ui_list.h"

using namespace Halley;

UIPopupMenu::UIPopupMenu(String id, UIStyle style, std::vector<UIPopupMenuItem> items)
	: UIWidget(std::move(id), {}, UISizer(UISizerType::Vertical, style.getFloat("gap")), style.getBorder("innerBorder"))
	, style(style)
	, items(std::move(items))
{
	makeUI();
}

void UIPopupMenu::update(Time t, bool moved)
{
	if (destroyOnUpdate) {
		destroy();
	}
}

void UIPopupMenu::pressMouse(Vector2f mousePos, int button)
{
	if (!getRect().contains(mousePos)) {
		destroy();
	}
}

void UIPopupMenu::onAddedToRoot(UIRoot& root)
{
	root.setFocus(itemList);
}

void UIPopupMenu::setInputButtons(const UIInputButtons& buttons)
{
	inputButtons = buttons;
	if (itemList) {
		itemList->setInputButtons(buttons);
	}
}

void UIPopupMenu::makeUI()
{
	itemList = std::make_shared<UIList>("items", style);
	for (const auto& item : items) {
		if (item.isSeparator) {
			itemList->add(std::make_shared<UIImage>(style.getSprite("separator")));
		} else {
			itemList->addTextIconItem(item.id, item.text, item.icon, -1, {}, UISizerFillFlags::Fill, item.tooltip)->setEnabled(item.enabled);
		}
	}

	itemList->setHandle(UIEventType::ListAccept, [=](const UIEvent& event) {
		sendEvent(UIEvent(UIEventType::PopupAccept, getId(), event.getStringData(), event.getIntData()));
		destroyOnUpdate = true;
	});

	itemList->setHandle(UIEventType::ListSelectionChanged, [=](const UIEvent& event) {
		sendEvent(UIEvent(UIEventType::PopupSelectionChanged, getId(), event.getStringData(), event.getIntData()));
	});

	itemList->setHandle(UIEventType::ListHoveredChanged, [=](const UIEvent& event) {
		sendEvent(UIEvent(UIEventType::PopupHoveredChanged, getId(), event.getStringData(), event.getIntData()));
	});
	
	add(itemList);

	itemList->setRequiresSelection(false);
	itemList->setSelectedOption(-1);
	itemList->setInputButtons(inputButtons);
	
	setHandle(UIEventType::UnhandledMousePressLeft, [=](const UIEvent&) {
		sendEvent(UIEvent(UIEventType::PopupCanceled, getId()));
		destroyOnUpdate = true;
	});

	setHandle(UIEventType::UnhandledMousePressMiddle, [=](const UIEvent&) {
		sendEvent(UIEvent(UIEventType::PopupCanceled, getId()));
		destroyOnUpdate = true;
	});

	setHandle(UIEventType::UnhandledMousePressRight, [=](const UIEvent&) {
		sendEvent(UIEvent(UIEventType::PopupCanceled, getId()));
		destroyOnUpdate = true;
	});

	itemList->setHandle(UIEventType::ListCancel, [=](const UIEvent&) {
		sendEvent(UIEvent(UIEventType::PopupCanceled, getId()));
		destroyOnUpdate = true;
	});
}
