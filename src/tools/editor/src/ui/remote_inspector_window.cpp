#include "remote_inspector_window.h"

using namespace Halley;

RemoteInspectorWindow::RemoteInspectorWindow(UIFactory& factory, std::shared_ptr<DevConServerConnection> connection, const HalleyAPI& api)
	: UIWidget("remote_inspector_window", {}, UISizer())
	, factory(factory)
	, connection(std::move(connection))
	, api(api)
{
	factory.loadUI(*this, "halley/remote_inspector_window");
}

void RemoteInspectorWindow::onMakeUI()
{
	// TODO
}

void RemoteInspectorWindow::onActiveChanged(bool active)
{
	// TODO
}

void RemoteInspectorWindow::update(Time t, bool moved)
{
	// TODO
}
