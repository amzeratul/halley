#include "widgets/ui_debug_console.h"
#include "widgets/ui_label.h"
#include "widgets/ui_textinput.h"
#include "widgets/ui_scrollbar_pane.h"
#include "ui_factory.h"

Halley::UIDebugConsole::UIDebugConsole(const String& id, UIFactory& factory)
	: UIWidget(id, {}, UISizer(UISizerType::Vertical))
	, factory(factory)
{
	setup();
}

void Halley::UIDebugConsole::show()
{
	setActive(true);
	getWidget("input")->setActive(true);
	getRoot()->setFocus(getWidget("input"));
}

void Halley::UIDebugConsole::hide()
{
	setActive(false);
	getWidget("input")->setActive(false);
	getRoot()->setFocus({});
}

void Halley::UIDebugConsole::setup()
{
	add(factory.makeUI("ui/halley/debug_console"));

	setHandle(UIEventType::ButtonClicked, [=] (const UIEvent& event)
	{
		onSubmit();
	});
	
	setHandle(UIEventType::TextSubmit, [=] (const UIEvent& event)
	{
		onSubmit();
	});
}

void Halley::UIDebugConsole::onSubmit()
{
	auto input = getWidgetAs<UITextInput>("input");
	runCommand(input->getText());
	input->setText("");
}

void Halley::UIDebugConsole::runCommand(const String& command)
{
	addLine(command);
	getWidget("input")->setFocused(true);
}

void Halley::UIDebugConsole::addLine(const String& line)
{
	auto style = factory.getStyle("label").getTextRenderer("label");
	auto newLabel = std::make_shared<UILabel>("", style, LocalisedString::fromUserString(line));
	auto scrollPane = getWidgetAs<UIScrollBarPane>("log");
	scrollPane->add(newLabel);
	scrollPane->getPane()->refresh();
	scrollPane->getPane()->setRelativeScroll(1.0f, UIScrollDirection::Vertical);
}
