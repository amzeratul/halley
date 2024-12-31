#include "remote_connection_window.h"

#include "console_window.h"
#include "halley/devcon/devcon_messages.h"

using namespace Halley;

RemoteConnectionWindow::RemoteConnectionWindow(UIFactory& factory, ProjectWindow& projectWindow, std::shared_ptr<DevConServerConnection> connection)
	: UIWidget("remote_tab", {}, UISizer())
	, factory(factory)
	, projectWindow(projectWindow)
	, connection(std::move(connection))
{
	factory.loadUI(*this, "halley/remote_connection_window");
}

void RemoteConnectionWindow::onMakeUI()
{
	console = std::make_shared<UIDebugConsole>("remoteConsole", factory, *this);

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
	auto response = UIDebugConsoleResponse("TODO", false);
	return Future<UIDebugConsoleResponse>::makeImmediate(response);
}

Vector<StringUTF32> RemoteConnectionWindow::getAutoComplete(const StringUTF32& line) const
{
	return {};
}
