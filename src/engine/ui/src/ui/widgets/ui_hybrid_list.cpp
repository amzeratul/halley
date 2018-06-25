#include "halley/ui/widgets/ui_hybrid_list.h"
#include "halley/ui/widgets/ui_label.h"
#include "ui_style.h"

using namespace Halley;

UIHybridList::UIHybridList(const String& id, UIStyle listStyle, UIStyle buttonStyle, UISizerType orientation, int nColumns)
	: UIWidget(id, {}, UISizer(orientation))
	, listStyle(listStyle)
	, buttonStyle(buttonStyle)
	, nColumns(nColumns)
{
	list = std::make_shared<UIList>(id + "_list", listStyle, orientation, nColumns);
	list->setOnlyEnabledWithInputs({ UIInputType::Gamepad });

	buttons = std::make_shared<UIWidget>(id + "_buttons", Vector2f(), UISizer(orientation, listStyle.getFloat("gap"), nColumns));
	buttons->setOnlyEnabledWithInputs({{ UIInputType::Mouse, UIInputType::Keyboard }});

	UIWidget::add(list);
	UIWidget::add(buttons);
}

void UIHybridList::addTextItem(const String& id, const LocalisedString& label)
{
	list->addTextItem(id, label, -1, true);

	auto button = std::make_shared<UIButton>(id, buttonStyle, UISizer(UISizerType::Vertical));
	button->add(std::make_shared<UILabel>("", buttonStyle.getTextRenderer("label"), label), 0, buttonStyle.getBorder("labelBorder"), UISizerAlignFlags::Centre);
	buttons->add(button);
}

void UIHybridList::addDivider()
{
	// TODO
}

void UIHybridList::setInputButtons(const UIInputButtons& buttons)
{
	list->setInputButtons(buttons);
}
