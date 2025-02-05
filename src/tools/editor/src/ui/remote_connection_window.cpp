#include "remote_connection_window.h"

#include "console_window.h"
#include "project_window.h"
#include "remote_inspector_window.h"
#include "remote_profiler_window.h"
#include "halley/devcon/devcon_messages.h"
#include "halley/tools/file/filesystem.h"

using namespace Halley;

RemoteConnectionTab::RemoteConnectionTab(UIFactory& factory, GamePlatform platform, String name)
	: UIWidget("remote_connection_tab", {}, UISizer())
	, factory(factory)
	, platform(platform)
	, name(std::move(name))
{
	factory.loadUI(*this, "halley/remote_tab_contents");
}

void RemoteConnectionTab::onMakeUI()
{
	getWidgetAs<UILabel>("label")->setText(LocalisedString::fromUserString(name));
	getWidgetAs<UIImage>("icon")->setSprite(Sprite().setImage(factory.getResources(), "ui/platforms/" + toString(platform) + "_16.png"));
}

RemoteConnectionWindow::RemoteConnectionWindow(UIFactory& factory, ProjectWindow& projectWindow, std::shared_ptr<DevConServerConnection> connection, std::shared_ptr<RemoteConnectionTab> tab)
	: UIWidget("remote_connection_window", {}, UISizer())
	, factory(factory)
	, projectWindow(projectWindow)
	, connection(std::move(connection))
	, tab(std::move(tab))
{
	makeCommands();
	factory.loadUI(*this, "halley/remote_connection_window");
}

void RemoteConnectionWindow::onMakeUI()
{
	colourScheme = factory.getColourScheme();

	console = std::make_shared<UIDebugConsole>("remoteConsole", factory, *this);
	console->setUserTextColour(colourScheme->getColour("ui_consoleUserText"), colourScheme->getColour("ui_consoleResponse"));

	auto inspector = std::make_shared<RemoteInspectorWindow>(factory, connection, projectWindow.getAPI());
	auto profiler = std::make_shared<RemoteProfilerWindow>(factory, connection, projectWindow.getAPI());

	getWidget("consoleRoot")->add(console, 1);
	getWidget("inspectorRoot")->add(inspector, 1);
	getWidget("profilerRoot")->add(profiler, 1);
}

void RemoteConnectionWindow::update(Time t, bool moved)
{
	for (auto& log: connection->movePendingLogs()) {
		addConsoleLine(log.level, log.msg);
	}
}

Future<UIDebugConsoleResponse> RemoteConnectionWindow::runCommand(String command, Vector<String> args)
{
	if (command != "help" && consoleController->hasCommand(command)) {
		return consoleController->runCommand(std::move(command), std::move(args));
	}

	ConfigNode params;
	params["command"] = std::move(command);
	params["args"] = std::move(args);
	return connection->sendRPC("consoleCommand", std::move(params)).then([] (ConfigNode result)
	{
		return UIDebugConsoleResponse(result);
	});
}

Future<Vector<StringUTF32>> RemoteConnectionWindow::getAutoComplete(const StringUTF32& line) const
{
	return connection->sendRPC("autoCompleteConsoleCommand", ConfigNode(String(line))).then([] (ConfigNode result) -> Vector<StringUTF32>
	{
		auto strs = result.asVector<String>({});
		Vector<StringUTF32> strs32;
		for (const auto& str: strs) {
			strs32.push_back(str.getUTF32());
		}
		return strs32;
	}).then([this, line] (Vector<StringUTF32> result) {
		for (auto& e: consoleController->getAutoComplete(line).get()) {
			result.push_back(e);
		}
		return result;
	});
}

void RemoteConnectionWindow::makeCommands()
{
	consoleCommands = std::make_shared<UIDebugConsoleCommands>();
	consoleController = std::make_shared<UIDebugConsoleController>();
	consoleController->addCommands(*consoleCommands);

	consoleCommands->addCommand("pullSave", [=] (Vector<String> args) -> String {
		ConfigNode params;
		params["name"] = args[0];
		connection->sendRPC("pullSave", std::move(params)).then([this, file = args[0]] (ConfigNode result) {
			saveRemoteSave(file, result.asBytes());
		});
		return "";
	});
	consoleCommands->addCommand("pushSave", [=] (Vector<String> args) -> String {
		loadRemoteSave(args[0]).then([this, file = args[0]] (Bytes bytes) {
			ConfigNode params;
			params["name"] = file;
			params["data"] = std::move(bytes);
			connection->sendRPC("pushSave", std::move(params));
		});
		return "";
	});
}

void RemoteConnectionWindow::saveRemoteSave(String name, Bytes data) const
{
	auto basePath = projectWindow.getAPI().core->getEnvironment().getDataPath() / "remote_saves" / projectWindow.getProject().getBinName() / ".";
	FileSystem::createDir(basePath);

	FileChooserParameters fileChooserParams;
	fileChooserParams.defaultPath = basePath;
	fileChooserParams.fileName = name;
	fileChooserParams.save = true;
	OS::get().openFileChooser(fileChooserParams).then([this, data = std::move(data)](std::optional<Path> path) {
		if (path) {
			addConsoleLine(LoggerLevel::Dev, "Saving " + String::prettySize(data.size()) + " to " + path->toString());
			Path::writeFile(*path, data);
		}
	});
}

Future<Bytes> RemoteConnectionWindow::loadRemoteSave(String name) const
{
	auto basePath = projectWindow.getAPI().core->getEnvironment().getDataPath() / "remote_saves" / projectWindow.getProject().getBinName() / ".";
	FileSystem::createDir(basePath);

	FileChooserParameters fileChooserParams;
	fileChooserParams.defaultPath = basePath;
	fileChooserParams.fileName = name;
	fileChooserParams.save = false;
	return OS::get().openFileChooser(fileChooserParams).then([this](std::optional<Path> path) -> Bytes {
		if (path) {
			auto data = Path::readFile(*path);
			addConsoleLine(LoggerLevel::Dev, "Loaded " + String::prettySize(data.size()) + " from " + path->toString());
			return data;
		}
		return {};
	});
}

void RemoteConnectionWindow::addConsoleLine(LoggerLevel level, const String& msg) const
{
	console->addLine(msg, ConsoleWindow::getColour(*colourScheme, level));
}
