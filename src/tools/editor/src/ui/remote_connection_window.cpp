#include "remote_connection_window.h"

#include "console_window.h"
#include "halley/devcon/devcon_messages.h"

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
}

RemoteConnectionWindow::RemoteConnectionWindow(UIFactory& factory, ProjectWindow& projectWindow, std::shared_ptr<DevConServerConnection> connection, std::shared_ptr<RemoteConnectionTab> tab)
	: UIWidget("remote_connection_window", {}, UISizer())
	, factory(factory)
	, projectWindow(projectWindow)
	, connection(std::move(connection))
	, tab(std::move(tab))
{
	factory.loadUI(*this, "halley/remote_connection_window");
}

void RemoteConnectionWindow::onMakeUI()
{
	const auto& colourScheme = factory.getColourScheme();

	console = std::make_shared<UIDebugConsole>("remoteConsole", factory, *this);
	console->setUserTextColour(colourScheme->getColour("ui_consoleUserText"), colourScheme->getColour("ui_consoleResponse"));

	getWidget("consoleRoot")->add(console, 1);
}

void RemoteConnectionWindow::update(Time t, bool moved)
{
	const auto& colourScheme = factory.getColourScheme();

	for (auto& log: connection->movePendingLogs()) {
		console->addLine(log.msg, ConsoleWindow::getColour(*colourScheme, log.level));
	}
}

Future<UIDebugConsoleResponse> RemoteConnectionWindow::runCommand(String command, Vector<String> args)
{
	ConfigNode params;
	params["command"] = std::move(command);
	params["args"] = std::move(args);
	return connection->sendRPC("consoleCommand", std::move(params)).then([] (ConfigNode result)
	{
		return UIDebugConsoleResponse(result);
	});
}

Vector<StringUTF32> RemoteConnectionWindow::getAutoComplete(const StringUTF32& line) const
{
	return {};
}
