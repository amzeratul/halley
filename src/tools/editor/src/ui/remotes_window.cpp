#include "remotes_window.h"

#include "project_window.h"
#include "remote_connection_window.h"

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

	setHandle(UIEventType::ListSelectionChanged, "tabs", [=](const UIEvent& event)
	{
		pages->setPage(event.getIntData());
	});

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
	Vector<std::shared_ptr<DevConServerConnection>> desiredConnections;
	for (const auto& conn: connections) {
		if (conn->getClientInfo()) {
			desiredTabs.push_back(conn->getId());
			desiredConnections.push_back(conn);
		}
	}

	if (desiredTabs != curTabs) {
		const int n = static_cast<int>(curTabs.size());
		for (int i = n; --i >= 0;) {
			if (!desiredTabs.contains(curTabs[i])) {
				pages->removePage(i);
				tabs->removeItem(i);
			}
		}

		for (size_t i = 0; i < desiredTabs.size(); ++i) {
			const auto id = desiredTabs[i];
			if (!curTabs.contains(id)) {
				auto conn = desiredConnections[i];

				const auto& info = *conn->getClientInfo();
				auto tab = std::make_shared<RemoteConnectionTab>(factory, info.platform, info.deviceName);

				tabs->addItem(toString(id), tab);
				pages->addPage()->add(std::make_shared<RemoteConnectionWindow>(factory, projectWindow, conn, tab), 1);
			}
		}

		pages->setPage(tabs->getSelectedOption());

		curTabs = desiredTabs;
	}
}
