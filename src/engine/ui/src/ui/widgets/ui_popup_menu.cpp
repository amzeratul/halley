#include "widgets/ui_popup_menu.h"
#include "widgets/ui_list.h"

using namespace Halley;

UIPopupMenu::UIPopupMenu(String id, UIStyle style, std::vector<UIPopupMenuItem> items)
	: UIWidget(std::move(id), {}, UISizer(UISizerType::Vertical, style.getFloat("gap")), style.getBorder("innerBorder"))
	, style(style)
	, items(std::move(items))
{
	makeUI();
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
		itemList->addTextItem(item.id, item.text, -1, false, item.tooltip);		
	}

	itemList->setHandle(UIEventType::ListAccept, [=](const UIEvent& event) {
		const auto& item = items.at(event.getIntData());
		item.callback();
		destroy();
	});
	add(itemList);

	itemList->setRequiresSelection(false);
	itemList->setSelectedOption(-1);
	itemList->setInputButtons(inputButtons);
	
	setHandle(UIEventType::UnhandledMousePressLeft, [=](const UIEvent&) {
		destroy();
	});

	setHandle(UIEventType::UnhandledMousePressMiddle, [=](const UIEvent&) {
		destroy();
	});

	setHandle(UIEventType::UnhandledMousePressRight, [=](const UIEvent&) {
		destroy();
	});

	itemList->setHandle(UIEventType::ListCancel, [=](const UIEvent&) {
		destroy();
	});
}
