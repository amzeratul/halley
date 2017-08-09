#include "halley/core/ui/widgets/ui_hybrid_list.h"
#include "ui/ui_style.h"

using namespace Halley;

UIHybridList::UIHybridList(const String& id, AddCallback callback, std::shared_ptr<UIStyle> style, UISizerType orientation, int nColumns, Maybe<Vector4f> innerBorder)
	: UIWidget(id, {}, UISizer(orientation))
	, style(style)
	, callback(callback)
	, nColumns(nColumns)
	, innerBorder(innerBorder)
{
	list = std::make_shared<UIList>(id + "_list", style, orientation, nColumns);
	list->setOnlyEnabledWithInputs({ UIInputType::Gamepad });

	buttons = std::make_shared<UIWidget>(id + "_buttons", Vector2f(), UISizer(orientation, style->getFloat("list.gap"), nColumns));
	buttons->setOnlyEnabledWithInputs({{ UIInputType::Mouse, UIInputType::Keyboard }});

	UIWidget::add(list);
	UIWidget::add(buttons);
}

void UIHybridList::addId(const String& id)
{
	list->addItem(id, callback(id, AddType::List), 0, innerBorder ? innerBorder.get() : style->getBorder("button.innerBorder"));

	auto button = std::make_shared<UIButton>(id, style, UISizer(UISizerType::Horizontal), innerBorder);
	button->add(callback(id, AddType::Button));
	buttons->add(button);
}

void UIHybridList::setInputButtons(const UIList::Buttons& button)
{
	list->setInputButtons(button);
}
