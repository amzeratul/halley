#include "halley/core/ui/widgets/ui_hybrid_list.h"
#include "ui/ui_style.h"

using namespace Halley;

UIHybridList::UIHybridList(const String& id, AddCallback callback, UIStyle listStyle, UIStyle buttonStyle, UISizerType orientation, int nColumns)
	: UIWidget(id, {}, UISizer(orientation))
	, listStyle(listStyle)
	, buttonStyle(buttonStyle)
	, callback(callback)
	, nColumns(nColumns)
{
	list = std::make_shared<UIList>(id + "_list", listStyle, orientation, nColumns);
	list->setOnlyEnabledWithInputs({ UIInputType::Gamepad });

	buttons = std::make_shared<UIWidget>(id + "_buttons", Vector2f(), UISizer(orientation, listStyle.getFloat("gap"), nColumns));
	buttons->setOnlyEnabledWithInputs({{ UIInputType::Mouse, UIInputType::Keyboard }});

	UIWidget::add(list);
	UIWidget::add(buttons);
}

void UIHybridList::addId(const String& id)
{
	list->addItem(id, callback(id, AddType::List));

	auto button = std::make_shared<UIButton>(id, buttonStyle, UISizer(UISizerType::Horizontal), buttonStyle.getBorder("innerBorder"));
	button->add(callback(id, AddType::Button));
	buttons->add(button);
}

void UIHybridList::setInputButtons(const UIInputButtons& buttons)
{
	list->setInputButtons(buttons);
}
