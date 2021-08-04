#include "widgets/ui_debug_console.h"
#include "widgets/ui_label.h"
#include "widgets/ui_textinput.h"
#include "widgets/ui_scrollbar_pane.h"
#include "ui_factory.h"
#include "halley/concurrency/concurrent.h"
#include "halley/utils/algorithm.h"

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
				const auto result = commandData.syntax.checkSyntax(command, args);
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

bool UIDebugConsoleSyntax::Arg::checkArgument(const String& arg) const
{
	if (type == "enum") {
		return std_ex::contains(validOptionsCallback(), arg);
	} else if (type == "int") {
		return arg.isInteger();
	} else if (type == "float") {
		return arg.isNumber();
	} else {
		return true;
	}
}

String UIDebugConsoleSyntax::Variant::getSyntax() const
{
	bool first = true;
	String result;
	for (const auto& arg: args) {
		if (!first) {
			result += " ";
		} else {
			first = false;
		}
		result += "<" + arg.name + ">";
	}
	return result;
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

std::optional<String> UIDebugConsoleSyntax::checkSyntax(const String& command, gsl::span<const String> args) const
{
	auto [curVariant, argN, argStart, invalidArg] = getVariantMatch(command, args, true);
	if (!curVariant || invalidArg) {
		String result;
		if (curVariant && invalidArg) {
			result += "Invalid argument \"" + args[invalidArg.value()] + "\" for <" + variants[curVariant.value()].args[invalidArg.value()].name + ">.";
		} else {
			result += "Invalid syntax.";
		}
		result += " Usage:";
		for (const auto& var: variants) {
			result += "\n  " + command + " " + var.getSyntax();
		}
		return result;
	}

	return {};
}

std::vector<StringUTF32> UIDebugConsoleSyntax::getAutoComplete(const StringUTF32& line32) const
{
	const auto line = String(line32);
	auto [curVariant, argN, argStart, invalidArg] = getVariantMatch(line, false);
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

UIDebugConsoleSyntax::VariantMatch UIDebugConsoleSyntax::getVariantMatch(const String& line, bool validate) const
{
	const auto lineSplit = line.split(' ');
	if (lineSplit.empty()) {
		return {};
	}
	return getVariantMatch(lineSplit[0], gsl::span<const String>(lineSplit).subspan(1), validate);
}

UIDebugConsoleSyntax::VariantMatch UIDebugConsoleSyntax::getVariantMatch(const String& command, gsl::span<const String> args, bool validate) const
{
	Expects(variants.size() <= 8);

	if (args.empty()) {
		return {};
	}

	std::array<bool, 8> validVariant;
	for (size_t i = 0; i < validVariant.size(); ++i) {
		validVariant[i] = i < variants.size();
	}

	// Check for valid variants
	OptionalLite<size_t> invalidArg;
	for (size_t i = 0; i < args.size(); ++i) {
		const auto& arg = args[i];
		bool validArg = false;
		
		for (size_t j = 0; j < variants.size(); ++j) {
			if (validVariant[j]) {
				const auto& variantArgs = variants[j].args;
				if (i < variantArgs.size()) {
					if (validate) {
						if (variantArgs[i].checkArgument(arg)) {
							validArg = true;
						}
					}
				} else {
					validVariant[j] = false;
				}
			}
		}

		if (validate && !validArg) {
			invalidArg = i;
			break;
		}
	}

	// Find the current variant
	OptionalLite<size_t> curVariant;
	if (!variants.empty()) {
		for (size_t i = 0; i < variants.size(); ++i) {
			if (validVariant[i]) {
				curVariant = i;
				break;
			}
		}
	}

	// Find start of cur argument
	size_t argStart = command.length() + 1;
	for (size_t i = 0; i < args.size() - 1; ++i) {
		argStart += args[i].length() + 1;
	}
	
	const auto argN = args.size() - 1;
	return VariantMatch{ curVariant, argN, argStart, invalidArg };
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
