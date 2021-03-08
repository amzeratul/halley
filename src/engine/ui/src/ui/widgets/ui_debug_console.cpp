#include "widgets/ui_debug_console.h"
#include "widgets/ui_label.h"
#include "widgets/ui_textinput.h"
#include "widgets/ui_scrollbar_pane.h"
#include "ui_factory.h"
#include "halley/concurrency/concurrent.h"

using namespace Halley;

UIDebugConsole::UIDebugConsole(const String& id, UIFactory& factory, std::shared_ptr<UIDebugConsoleController> controller)
	: UIWidget(id, {}, UISizer(UISizerType::Vertical))
	, factory(factory)
	, controller(std::move(controller))
{
	setup();
}

UIDebugConsoleResponse::UIDebugConsoleResponse()
{
}

UIDebugConsoleResponse::UIDebugConsoleResponse(String response, bool closeConsole)
	: response(response)
	, closeConsole(closeConsole)
{
}

const String& UIDebugConsoleResponse::getResponse() const
{
	return response;
}

bool UIDebugConsoleResponse::isCloseConsole() const
{
	return closeConsole;
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

Future<UIDebugConsoleResponse> UIDebugConsoleController::runCommand(String command, std::vector<String> args)
{
	if (command == "help") {
		Promise<UIDebugConsoleResponse> value;
		value.setValue(runHelp());
		return value.getFuture();
	}
	
	for (auto& commandSet: commands) {
		const auto& cs = commandSet->getCommands();
		const auto iter = cs.find(command);
		if (iter != cs.end()) {
			const UIDebugConsoleCallbackPair& pair = iter->second;
			if (pair.first) {
				return Concurrent::execute(*pair.first, [args=std::move(args), f=pair.second] () -> UIDebugConsoleResponse {
					return f(args);
				});
			} else {
				Promise<UIDebugConsoleResponse> value;
				value.setValue(pair.second(args));
				return value.getFuture();
			}
		}
	}
	Promise<UIDebugConsoleResponse> value;
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

void UIDebugConsoleController::clearCommands()
{
	commands.clear();
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
	if (getRoot()) {
		getRoot()->setFocus(getWidget("input"));
	}
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
		return controller->getAutoComplete(str);
	});

	setHandle(UIEventType::ButtonClicked, "ok", [=] (const UIEvent& event)
	{
		getWidgetAs<UITextInput>("input")->submit();
	});
	
	setHandle(UIEventType::TextSubmit, "input", [=] (const UIEvent& event)
	{
		runCommand(event.getStringData());
	});

	layout();
}

void UIDebugConsole::runCommand(const String& rawCommand)
{
	addLine("> " + rawCommand, Colour::fromString("#FFFFFF"));
	inputField->setEnabled(false);

	auto args = rawCommand.split(' ');
	String command = std::move(args[0]);
	args.erase(args.begin());
	
	controller->runCommand(std::move(command), std::move(args)).then(Executors::getMainThread(), [=] (UIDebugConsoleResponse result) {
		if (!result.getResponse().isEmpty()) {
			addLine(result.getResponse(), Colour::fromString("#E2D5EA"));
		}
		inputField->setEnabled(true);
		getRoot()->setFocus(inputField);
		if (result.isCloseConsole()) {
			hide();
		}
	});
}

void UIDebugConsole::addLine(const String& line, Colour colour)
{
	auto style = factory.getStyle("label").getTextRenderer("label");
	style.setSize(16).setColour(colour);
	auto newLabel = std::make_shared<UILabel>("", style, LocalisedString::fromUserString(line));
	newLabel->setFlowLayout(true);
	auto scrollPane = getWidgetAs<UIScrollBarPane>("log");
	scrollPane->add(newLabel);
	scrollPane->getPane()->refresh();
	scrollPane->getPane()->setRelativeScroll(1.0f, UIScrollDirection::Vertical);
}

void UIDebugConsole::setForcePaintMask(int mask)
{
	forceMask = mask;
}

const std::shared_ptr<UIDebugConsoleController>& UIDebugConsole::getController() const
{
	return controller;
}

void UIDebugConsole::onAddedToRoot(UIRoot& root)
{
	if (isActive()) {
		root.setFocus(getWidget("input"));
	}
}

void UIDebugConsole::drawChildren(UIPainter& painter) const
{
	if (forceMask) {
		auto p2 = painter.withMask(forceMask.value());
		UIWidget::drawChildren(p2);
	} else {
		UIWidget::drawChildren(painter);
	}
}
