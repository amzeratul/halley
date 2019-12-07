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
	if (command == "help") {
		Promise<String> value;
		value.setValue(runHelp());
		return value.getFuture();
	}
	
	for (auto& commandSet: commands) {
		const auto& cs = commandSet->getCommands();
		const auto iter = cs.find(command);
		if (iter != cs.end()) {
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
	}
	Promise<String> value;
	value.setValue("Command not found: \"" + command + "\".");
	return value.getFuture();
}

String UIDebugConsoleController::runHelp()
{
	String result = "Commands available:";
	for (auto& commandSet: commands) {
		for (auto& command: commandSet->getCommands()) {
			result += "\n  " + command.first;
		}
	}
	return result;
}

void UIDebugConsoleController::addCommands(UIDebugConsoleCommands& commandSet)
{
	commands.push_back(&commandSet);
}

void UIDebugConsoleController::removeCommands(UIDebugConsoleCommands& commandSet)
{
	commands.erase(std::remove(commands.begin(), commands.end(), &commandSet), commands.end());
}

std::vector<StringUTF32> UIDebugConsoleController::getAutoComplete(const StringUTF32& line) const
{
	std::vector<StringUTF32> results;

	auto tryCommand = [&] (const StringUTF32& c)
	{
		if (c.substr(0, line.size()) == line) {
			results.push_back(c);
		}
	};

	tryCommand(String("help").getUTF32());
	for (auto& commandSet: commands) {
		for (auto& command: commandSet->getCommands()) {
			tryCommand(command.first.getUTF32());
		}
	}
	return results;
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
	add(factory.makeUI("ui/halley/debug_console"), 1);

	inputField = getWidgetAs<UITextInput>("input");
	inputField->setAutoCompleteHandle([=] (const StringUTF32& str) -> std::vector<StringUTF32>
	{
		return controller.getAutoComplete(str);
	});

	setHandle(UIEventType::ButtonClicked, "ok", [=] (const UIEvent& event)
	{
		onSubmit();
	});
	
	setHandle(UIEventType::TextSubmit, "input", [=] (const UIEvent& event)
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
