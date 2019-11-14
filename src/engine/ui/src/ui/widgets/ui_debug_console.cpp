#include "widgets/ui_debug_console.h"
#include "widgets/ui_label.h"
#include "widgets/ui_textinput.h"
#include "widgets/ui_scrollbar_pane.h"
#include "ui_factory.h"
#include "halley/concurrency/concurrent.h"

using namespace Halley;

UIDebugConsole::UIDebugConsole(const String& id, UIFactory& factory, UIDebugConsoleController& controller)
	: UIWidget(id, {}, UISizer(UISizerType::Vertical))
	, factory(factory)
	, controller(controller)
{
	setup();
}

void UIDebugConsoleCommands::addCommand(String command, UIDebugConsoleCallback callback)
{
	commands[command] = UIDebugConsoleCallbackPair(nullptr, std::move(callback));
}

void UIDebugConsoleCommands::addAsyncCommand(String command, ExecutionQueue& queue, UIDebugConsoleCallback callback)
{
	commands[command] = UIDebugConsoleCallbackPair(&queue, std::move(callback));
}

const std::map<String, UIDebugConsoleCallbackPair>& UIDebugConsoleCommands::getCommands() const
{
	return commands;
}

Future<String> UIDebugConsoleController::runCommand(String command, std::vector<String> args)
{
	const auto iter = commands.find(command);
	if (iter != commands.end()) {
		const UIDebugConsoleCallbackPair& pair = iter->second;
		if (pair.first) {
			return Concurrent::execute(*pair.first, [args=std::move(args), f=pair.second] () -> String {
				return f(args);
			});
		} else {
			Promise<String> value;
			value.setValue(pair.second(args));
			return value.getFuture();
		}
	}
	Promise<String> value;
	value.setValue("Command not found: \"" + command + "\".");
	return value.getFuture();
}

void UIDebugConsoleController::addCommands(UIDebugConsoleCommands& commands)
{
	for (auto& c: commands.getCommands()) {
		if (this->commands.find(c.first) == this->commands.end()) {
			this->commands[c.first] = c.second;
		} else {
			throw Exception("Duplicated debug console command: " + c.first, HalleyExceptions::UI);
		}
	}
}

void UIDebugConsoleController::removeCommands(UIDebugConsoleCommands& commands)
{
	for (auto& c: commands.getCommands()) {
		this->commands.erase(c.first);
	}
}

void UIDebugConsole::show()
{
	setActive(true);
	inputField->setActive(true);
	getRoot()->setFocus(getWidget("input"));
}

void UIDebugConsole::hide()
{
	setActive(false);
	inputField->setActive(false);
	getRoot()->setFocus({});
}

void UIDebugConsole::setup()
{
	add(factory.makeUI("ui/halley/debug_console"));

	inputField = getWidgetAs<UITextInput>("input");

	setHandle(UIEventType::ButtonClicked, [=] (const UIEvent& event)
	{
		onSubmit();
	});
	
	setHandle(UIEventType::TextSubmit, [=] (const UIEvent& event)
	{
		onSubmit();
	});
}

void UIDebugConsole::onSubmit()
{
	auto input = getWidgetAs<UITextInput>("input");
	runCommand(input->getText());
	input->setText("");
}

void UIDebugConsole::runCommand(const String& rawCommand)
{
	addLine("> " + rawCommand, Colour::fromString("#FFFFFF"));
	inputField->setEnabled(false);

	auto args = rawCommand.split(' ');
	String command = std::move(args[0]);
	args.erase(args.begin());
	
	controller.runCommand(std::move(command), std::move(args)).then(Executors::getMainThread(), [=] (String result) {
		addLine(result, Colour::fromString("#E2D5EA"));
		inputField->setEnabled(true);
		inputField->setFocused(true);
	});
}

void UIDebugConsole::addLine(const String& line, Colour colour)
{
	auto style = factory.getStyle("label").getTextRenderer("label");
	style.setSize(16).setColour(colour);
	auto newLabel = std::make_shared<UILabel>("", style, LocalisedString::fromUserString(line));
	auto scrollPane = getWidgetAs<UIScrollBarPane>("log");
	scrollPane->add(newLabel);
	scrollPane->getPane()->refresh();
	scrollPane->getPane()->setRelativeScroll(1.0f, UIScrollDirection::Vertical);
}
