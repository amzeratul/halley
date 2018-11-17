#include "halley/ui/widgets/ui_hybrid_list.h"
#include "halley/ui/widgets/ui_label.h"
#include "ui_style.h"
#include "widgets/ui_image.h"

using namespace Halley;

UIHybridList::UIHybridList(const String& id, UIStyle style, UISizerType orientation, int nColumns)
	: UIWidget(id, {}, UISizer(orientation))
	, style(style)
	, nColumns(nColumns)
{
	auto listStyle = style.getSubStyle("list");
	list = std::make_shared<UIList>(id + "_list", listStyle, orientation, nColumns);
	list->setOnlyEnabledWithInputs({ UIInputType::Gamepad });

	buttons = std::make_shared<UIWidget>(id + "_buttons", Vector2f(), UISizer(orientation, listStyle.getFloat("gap"), nColumns));
	buttons->setOnlyEnabledWithInputs({{ UIInputType::Mouse, UIInputType::Keyboard }});

	UIWidget::add(list);
	UIWidget::add(buttons);

	setHandle(UIEventType::ButtonClicked, [=] (const UIEvent& event)
	{
		sendEvent(UIEvent(UIEventType::ListAccept, event.getCurWidget().getId(), event.getSourceId()));
	});

	setHandle(UIEventType::ListCancel, [=] (const UIEvent& event)
	{
		sendEvent(UIEvent(UIEventType::ListAccept, event.getCurWidget().getId(), String("cancel")));
	});
}

void UIHybridList::addTextItem(const String& id, const LocalisedString& label)
{
	list->addTextItem(id, label, -1, true);

	auto buttonStyle = style.getSubStyle("button");
	auto button = std::make_shared<UIButton>(id, buttonStyle, UISizer(UISizerType::Vertical));

	if (id == "cancel") {
		cancelButton = button;
		UIInputButtons inputButtons;
		inputButtons.accept = uiInputButtons.cancel;
		cancelButton->setInputButtons(inputButtons);
	}

	button->add(std::make_shared<UILabel>("", buttonStyle.getTextRenderer("label"), label), 0, buttonStyle.getBorder("labelBorder"), UISizerAlignFlags::Centre);
	buttons->add(button);
}

void UIHybridList::addDivider()
{
	auto dividerStyle = style.getSubStyle("divider");
	buttons->add(std::make_shared<UIImage>(dividerStyle.getSprite("image")), 0, dividerStyle.getBorder("border"));
	list->add(std::make_shared<UIImage>(dividerStyle.getSprite("image")), 0, dividerStyle.getBorder("border"));
}

void UIHybridList::setInputButtons(const UIInputButtons& inputButtons)
{
	list->setInputButtons(inputButtons);
	if (cancelButton) {
		UIInputButtons buttonInputButtons;
		buttonInputButtons.accept = inputButtons.cancel;
		cancelButton->setInputButtons(buttonInputButtons);
	}
	uiInputButtons = inputButtons;
}

void UIHybridList::setItemEnabled(const String& id, bool enabled)
{
	buttons->getWidget(id)->setEnabled(enabled);
	list->getItem(id)->setEnabled(enabled);
}
