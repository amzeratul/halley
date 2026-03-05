#include "halley/ui/widgets/ui_debug_console.h"
#include "halley/ui/widgets/ui_label.h"
#include "halley/ui/widgets/ui_textinput.h"
#include "halley/ui/widgets/ui_scrollbar_pane.h"
#include "halley/ui/ui_factory.h"
#include "halley/concurrency/concurrent.h"
#include "halley/api/halley_api.h"
#include "halley/utils/algorithm.h"
#include "halley/audio/audio_event.h"

using namespace Halley;


UIDebugConsole::UIDebugConsole(const String& id, UIFactory& factory)
	: UIWidget(id, {}, UISizer(UISizerType::Vertical))
	, factory(factory)
{
	setup();
}

UIDebugConsole::UIDebugConsole(const String& id, UIFactory& factory, IUIDebugConsoleController& controller)
	: UIWidget(id, {}, UISizer(UISizerType::Vertical))
	, factory(factory)
	, controller(&controller)
{
	setup();
}

UIDebugConsoleResponse::UIDebugConsoleResponse()
{
}

UIDebugConsoleResponse::UIDebugConsoleResponse(String response, bool closeConsole, bool error)
	: response(std::move(response))
	, closeConsole(closeConsole)
	, error(error)
{
}

UIDebugConsoleResponse::UIDebugConsoleResponse(const char* response)
	: response(response)
{
}

UIDebugConsoleResponse::UIDebugConsoleResponse(const ConfigNode& node)
{
	if (node.getType() == ConfigNodeType::String) {
		response = node.asString();
	} else {
		response = node["response"].asString("");
		closeConsole = node["closeConsole"].asBool(false);
	}
}

const String& UIDebugConsoleResponse::getResponse() const
{
	return response;
}

bool UIDebugConsoleResponse::isCloseConsole() const
{
	return closeConsole;
}

bool UIDebugConsoleResponse::hasError() const
{
	return error;
}

ConfigNode UIDebugConsoleResponse::toConfigNode() const
{
	ConfigNode result;
	if (!response.isEmpty()) {
		result["response"] = response;
	}
	if (closeConsole) {
		result["closeConsole"] = closeConsole;
	}
	return result;
}

void UIDebugConsoleCommands::addCommand(String command, UIDebugConsoleCallback callback)
{
	commands[command.asciiLower()] = UIDebugConsoleCommandData{ command, std::move(callback), nullptr, {} };
}

void UIDebugConsoleCommands::addCommand(String command, UIDebugConsoleCallback callback, UIDebugConsoleSyntax syntax)
{
	commands[command.asciiLower()] = UIDebugConsoleCommandData{ command, std::move(callback), nullptr, std::move(syntax) };
}

void UIDebugConsoleCommands::addAsyncCommand(String command, ExecutionQueue& queue, UIDebugConsoleCallback callback)
{
	commands[command.asciiLower()] = UIDebugConsoleCommandData{ command, std::move(callback), &queue, {} };
}

void UIDebugConsoleCommands::addAsyncCommand(String command, ExecutionQueue& queue, UIDebugConsoleCallback callback, UIDebugConsoleSyntax syntax)
{
	commands[command.asciiLower()] = UIDebugConsoleCommandData{ command, std::move(callback), &queue, std::move(syntax) };
}

void UIDebugConsoleCommands::addCommandBatches(const ConfigNode& node, IUIDebugConsoleController& controller)
{
	for (const auto& [commandId, commandsNode]: node.asMap()) {
		addCommand(commandId, [&controller, commands = commandsNode.asVector<String>({})] (Vector<String> _args) -> UIDebugConsoleResponse {
			Vector<String> outputs;

			for (const auto& rawCommand: commands) {
				auto args = rawCommand.split(' ');
				String command = std::move(args[0]);
				args.erase(args.begin());

				auto result = controller.runCommand(std::move(command), std::move(args)).get();
				if (result.hasError()) {
					outputs += result.getResponse();
				}
			}

			if (outputs.empty()) {
				return UIDebugConsoleResponse("Done");
			} else {
				return UIDebugConsoleResponse(String::concatList(outputs.const_span(), "\n"), false, true);
			}
		}, { UIDebugConsoleSyntax() });
	}
}

void UIDebugConsoleCommands::removeCommand(const String& command)
{
	commands.erase(command.asciiLower());
}

const std::map<String, UIDebugConsoleCommandData>& UIDebugConsoleCommands::getCommands() const
{
	return commands;
}

void UIDebugConsoleCommands::clear()
{
	commands.clear();
}

UIDebugConsoleController::UIDebugConsoleController()
{
	baseCommandSet = std::make_unique<UIDebugConsoleCommands>();
	
	baseCommandSet->addCommand("help", [=](Vector<String>)
	{
		return runHelp();
	});
}

UIDebugConsoleController::UIDebugConsoleController(Resources& resources, const HalleyAPI& api)
{
	baseCommandSet = std::make_unique<UIDebugConsoleCommands>();
	
	baseCommandSet->addCommand("help", [=](Vector<String>)
	{
		return runHelp();
	});
	
	// TODO: move these two out of here?
	baseCommandSet->addCommand("audioGlobalSwitch", [&api] (Vector<String> args) -> String
	{
		if (args.size() != 2) {
			return "Usage: audioGlobalSwitch <switchId> <value>";
		} else {
			api.audio->getGlobalEmitter()->setSwitch(args[0], args[1]);
			return "Switch set.";
		}
	});

	baseCommandSet->addCommand("audioGlobalEvent", [&api, &resources] (Vector<String> args) -> String
	{
		if (args.size() != 1) {
			return "Usage: audioGlobalEvent <eventId>";
		} else {
			if (resources.exists<AudioEvent>(args[0])) {
				api.audio->postEvent(args[0]);
				return "Posted event.";
			} else {
				return "Event not found.";
			}
		}		
	});

	clearCommands();
}

Future<UIDebugConsoleResponse> UIDebugConsoleController::runCommand(String command, Vector<String> args)
{
	for (auto& commandSet: commands) {
		const auto& cs = commandSet->getCommands();
		const auto iter = cs.find(command.asciiLower());
		if (iter != cs.end()) {
			return runCommand(command, iter->second, std::move(args));
		}
	}
	Promise<UIDebugConsoleResponse> value;
	value.setValue(UIDebugConsoleResponse("Command not found: \"" + command + "\".", false, true));
	return value.getFuture();
}

bool UIDebugConsoleController::hasCommand(const String& command) const
{
	for (auto& commandSet: commands) {
		const auto& cs = commandSet->getCommands();
		if (cs.find(command.asciiLower()) != cs.end()) {
			return true;
		}
	}
	return false;
}

Future<UIDebugConsoleResponse> UIDebugConsoleController::runCommand(String command, const UIDebugConsoleCommandData& commandData, Vector<String> args)
{
	if (commandData.syntax.hasSyntax()) {
		const auto result = commandData.syntax.checkSyntax(command, args);
		if (result) {
			// Syntax error
			Promise<UIDebugConsoleResponse> value;
			value.setValue(UIDebugConsoleResponse(result.value(), false, true));
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

String UIDebugConsoleController::runHelp()
{
	String result = "Commands available:";
	for (auto& commandSet: commands) {
		for (auto& command: commandSet->getCommands()) {
			result += "\n  " + command.second.command;
		}
	}
	return result;
}

void UIDebugConsoleController::addCommands(UIDebugConsoleCommands& commandSet)
{
	if (!std_ex::contains(commands, &commandSet)) {
		commands.push_back(&commandSet);
	}
}

void UIDebugConsoleController::removeCommands(UIDebugConsoleCommands& commandSet)
{
	std_ex::erase(commands, &commandSet);
}

void UIDebugConsoleController::clearCommands()
{
	HalleyAssertDev(baseCommandSet != nullptr);
	commands.clear();
	addCommands(*baseCommandSet);
}

Future<Vector<StringUTF32>> UIDebugConsoleController::getAutoComplete(const StringUTF32& line) const
{
	Vector<StringUTF32> results;
	
	for (auto& commandSet: commands) {
		for (auto& command: commandSet->getCommands()) {
			const auto& c = command.second.command.getUTF32();
			if (line.size() > c.size() && line.substr(0, c.size()) == c && line[c.size()] == ' ') {
				// Line starts with command, autocomplete for it
				return Future<Vector<StringUTF32>>::makeImmediate(command.second.syntax.getAutoComplete(line));
			}
			if (c.substr(0, line.size()) == line) {
				// This command could be an autocomplete
				results.push_back(c);
			}
		}
	}
	return Future<Vector<StringUTF32>>::makeImmediate(results);
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

Vector<StringUTF32> UIDebugConsoleSyntax::getAutoComplete(const StringUTF32& line32) const
{
	const auto line = String(line32);
	auto [curVariant, argN, argStart, invalidArg] = getVariantMatch(line, false);
	if (!curVariant) {
		return {};
	}
	const auto curArg = line.substr(argStart);
	const auto& curArgSyntax = variants[curVariant.value()].args[argN];

	// Retrieve valid options
	Vector<String> validOptions;
	if (curArgSyntax.validOptionsCallback) {
		validOptions = curArgSyntax.validOptionsCallback();
	}

	// Filter matching ones
	const StringUTF32 linePrefix = line.substr(0, argStart).getUTF32();
	Vector<StringUTF32> results;
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
	HalleyAssertDev(variants.size() <= 8);
	
	std::array<bool, 8> validVariant;
	for (size_t i = 0; i < validVariant.size(); ++i) {
		validVariant[i] = i < variants.size() && (!validate || args.size() == variants[i].args.size());
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
	for (int i = 0; i < static_cast<int>(args.size()) - 1; ++i) {
		argStart += args[i].length() + 1;
	}
	
	const auto argN = args.size() - 1;
	return VariantMatch{ curVariant, argN, argStart, invalidArg };
}

void UIDebugConsole::setController(IUIDebugConsoleController* controller)
{
	this->controller = controller;
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
	userInputColour = Colour::fromString("#FFFFFF");
	responseColour = Colour::fromString("#E2D5EA");
	errorResponseColour = Colour::fromString("#E76D6D");

	add(factory.makeUI("halley/debug_console"), 1);

	inputField = getWidgetAs<UITextInput>("input");
	inputField->setAutoCompleteHandle([=] (const StringUTF32& str) -> Future<Vector<StringUTF32>>
	{
		if (controller) {
			return controller->getAutoComplete(str);
		} else {
			return Future<Vector<StringUTF32>>::makeImmediate({});
		}
	});

	setHandle(UIEventType::ButtonClicked, "ok", [=] (const UIEvent& event)
	{
		getWidgetAs<UITextInput>("input")->submit();
	});
	
	setHandle(UIEventType::TextSubmit, "input", [=] (const UIEvent& event)
	{
		auto cmd = event.getStringData();
		Concurrent::execute(Executors::getMainUpdateThread(), [=]() {
			if (controller) {
				runCommand(cmd);
			}
		});
	});

	layout();
}

void UIDebugConsole::runCommand(const String& rawCommand)
{
	addLine("> " + rawCommand, userInputColour);
	inputField->setEnabled(false);

	auto args = rawCommand.split(' ');
	String command = std::move(args[0]);
	args.erase(args.begin());
	std_ex::erase_if(args, [](const auto& arg) {return arg.isEmpty();});
	
	controller->runCommand(std::move(command), std::move(args)).then(Executors::getMainUpdateThread(), [=] (UIDebugConsoleResponse result) {
		if (!result.getResponse().isEmpty()) {
			addLine(result.getResponse(), result.hasError() ? errorResponseColour : responseColour);
		}
		inputField->setEnabled(true);
		getRoot()->setFocus(inputField);
		if (result.isCloseConsole() && autoHide) {
			hide();
		}
	});
}

void UIDebugConsole::addLine(const String& line, Colour colour)
{
	auto renderer = factory.getStyle("label").getTextRenderer("label");
	renderer.setSize(16).setColour(colour);
	
	auto newLabel = std::make_shared<UILabel>("", factory.getStyle("label"), renderer, LocalisedString::fromUserString(line));
	newLabel->setFlowLayout(true);
	auto scrollPane = getWidgetAs<UIScrollBarPane>("log");
	scrollPane->add(newLabel);
	scrollPane->getPane()->refresh();
	scrollPane->getPane()->setRelativeScroll(1.0f, UIScrollDirection::Vertical);
}

void UIDebugConsole::setUserTextColour(Colour4f userInputColour, Colour4f responseColour)
{
	this->userInputColour = userInputColour;
	this->responseColour = responseColour;
}

bool UIDebugConsole::canAutoHide() const
{
	return autoHide;
}

void UIDebugConsole::setAutoHide(bool enabled)
{
	autoHide = enabled;
}

void UIDebugConsole::setForcePaintMask(int mask)
{
	forceMask = mask;
}

void UIDebugConsole::onAddedToRoot(UIRoot& root)
{
	if (isActive()) {
		root.setFocus(getWidget("input"));
	}
}

bool UIDebugConsole::onKeyPress(KeyboardKeyPress key)
{
	return true;
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
