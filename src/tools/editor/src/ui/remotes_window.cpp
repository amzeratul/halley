#include "remotes_window.h"

#include "project_window.h"

using namespace Halley;

RemotesWindow::RemotesWindow(UIFactory& factory, ProjectWindow& projectWindow)
	: UIWidget("remotes_window", {}, UISizer())
	, factory(factory)
	, projectWindow(projectWindow)
	, devConServer(*projectWindow.getProject().getDevConServer())
{
	factory.loadUI(*this, "halley/remotes_window");
}

void RemotesWindow::onMakeUI()
{
	hasRemoteConnections = getWidget("hasRemoteConnections");
	noRemoteConnections = getWidget("noRemoteConnections");
	tabs = getWidgetAs<UIList>("tabs");
	pages = getWidgetAs<UIPagedPane>("pages");

	updateTabs();
}

void RemotesWindow::update(Time t, bool moved)
{
	updateTabs();
}

void RemotesWindow::updateTabs()
{
	const auto connections = devConServer.getConnections();
	hasRemoteConnections->setActive(!connections.empty());
	noRemoteConnections->setActive(connections.empty());

	Vector<size_t> desiredTabs;
	for (const auto& conn: connections) {
		desiredTabs.push_back(conn->getId());
	}

	if (desiredTabs != curTabs) {
		
	}
}
