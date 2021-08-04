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
	commands[command] = UIDebugConsoleCommandData{ std::move(callback), nullptr, {} };
}

void UIDebugConsoleCommands::addCommand(String command, UIDebugConsoleCallback callback, UIDebugConsoleSyntax syntax)
{
	commands[command] = UIDebugConsoleCommandData{ std::move(callback), nullptr, std::move(syntax) };
}

void UIDebugConsoleCommands::addAsyncCommand(String command, ExecutionQueue& queue, UIDebugConsoleCallback callback)
{
	commands[command] = UIDebugConsoleCommandData{ std::move(callback), &queue, {} };
}

void UIDebugConsoleCommands::addAsyncCommand(String command, ExecutionQueue& queue, UIDebugConsoleCallback callback, UIDebugConsoleSyntax syntax)
{
	commands[command] = UIDebugConsoleCommandData{ std::move(callback), &queue, std::move(syntax) };
}

const std::map<String, UIDebugConsoleCommandData>& UIDebugConsoleCommands::getCommands() const
{
	return commands;
}

UIDebugConsoleController::UIDebugConsoleController()
{
	baseCommandSet = std::make_unique<UIDebugConsoleCommands>();
	baseCommandSet->addCommand("help", [=](std::vector<String>) { return runHelp(); });
	clearCommands();
}

Future<UIDebugConsoleResponse> UIDebugConsoleController::runCommand(String command, std::vector<String> args)
{
	for (auto& commandSet: commands) {
		const auto& cs = commandSet->getCommands();
		const auto iter = cs.find(command);
		if (iter != cs.end()) {
			const UIDebugConsoleCommandData& commandData = iter->second;

			if (commandData.syntax.hasSyntax()) {
				const auto result = commandData.syntax.checkSyntax(command);
				if (result) {
					// Syntax error
					Promise<UIDebugConsoleResponse> value;
					value.setValue(result.value());
					return value.getFuture();
				}
			}
			
			if (commandData.queue) {
				return Concurrent::execute(*commandData.queue, [args=std::move(args), f=commandData.callback] () -> UIDebugConsoleResponse {
					return f(args);
				});
			} else {
				Promise<UIDebugConsoleResponse> value;
				value.setValue(commandData.callback(args));
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
	Expects(baseCommandSet != nullptr);
	commands.clear();
	addCommands(*baseCommandSet);
}

std::vector<StringUTF32> UIDebugConsoleController::getAutoComplete(const StringUTF32& line) const
{
	std::vector<StringUTF32> results;
	
	for (auto& commandSet: commands) {
		for (auto& command: commandSet->getCommands()) {
			const auto& c = command.first.getUTF32();
			if (line.size() > c.size() && line.substr(0, c.size()) == c && line[c.size()] == ' ') {
				// Line starts with command, autocomplete for it
				return command.second.syntax.getAutoComplete(line);
			}
			if (c.substr(0, line.size()) == line) {
				// This command could be an autocomplete
				results.push_back(c);
			}
		}
	}
	return results;
}

UIDebugConsoleSyntax::UIDebugConsoleSyntax(std::initializer_list<Arg> args)
{
	variants.emplace_back(std::move(args));
}

UIDebugConsoleSyntax::UIDebugConsoleSyntax(std::initializer_list<Variant> variants)
	: variants(std::move(variants))
{
}

bool UIDebugConsoleSyntax::hasSyntax() const
{
	return !variants.empty();
}

std::optional<String> UIDebugConsoleSyntax::checkSyntax(const String& line) const
{
	// TODO
	return {};
}

std::vector<StringUTF32> UIDebugConsoleSyntax::getAutoComplete(const StringUTF32& line32) const
{
	const auto line = String(line32);
	auto [curVariant, argN, argStart] = getVariantMatch(line);
	if (!curVariant) {
		return {};
	}
	const auto curArg = line.substr(argStart);
	const auto& curArgSyntax = variants[curVariant.value()].args[argN];

	// Retrieve valid options
	std::vector<String> validOptions;
	if (curArgSyntax.validOptionsCallback) {
		validOptions = curArgSyntax.validOptionsCallback();
	}

	// Filter matching ones
	const StringUTF32 linePrefix = line.substr(0, argStart).getUTF32();
	std::vector<StringUTF32> results;
	for (const auto& o: validOptions) {
		if (o.startsWith(curArg)) {
			results.emplace_back(linePrefix + o.getUTF32());
		}
	}
	
	return results;
}

UIDebugConsoleSyntax::VariantMatch UIDebugConsoleSyntax::getVariantMatch(const String& line) const
{
	// Count which argument number we're at, and where it starts
	size_t argN = 0;
	size_t argStart = 0;
	for (size_t i = 0; i < line.size(); ++i) {
		if (line[i] == ' ') {
			++argN;
			argStart = i + 1;
		}
	}
	if (argN == 0) {
		return {};
	}
	--argN; // Don't count the command itself

	// Find the current variant
	OptionalLite<size_t> curVariant;
	if (!variants.empty()) {
		for (size_t i = 0; i < variants.size(); ++i) {
			if (argN < variants[i].args.size()) {
				curVariant = i;
				break;
			}
		}
	}

	return VariantMatch{ curVariant, argN, argStart };
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
