#include "remotes_status_bar.h"

using namespace Halley;

RemotesStatusBar::RemotesStatusBar(UIFactory& factory, ProjectWindow& projectWindow)
	: UIWidget("remote_status_bar", {}, UISizer())
	, factory(factory)
	, projectWindow(projectWindow)
{
	factory.loadUI(*this, "halley/remote_status_bar");

	setInteractWithMouse(true);
}

void RemotesStatusBar::onMakeUI()
{
	connectionsActive = getWidgetAs<UILabel>("connectionsActive");

	const auto& cs = factory.getColourScheme();
	activeCol = cs->getColour("ui_remotes_active");
	inactiveCol = cs->getColour("ui_remotes_inactive");
}

void RemotesStatusBar::update(Time t, bool moved)
{
	const auto& conns = projectWindow.getProject().getDevConServer()->getConnections();
	connectionsActive->setText(LocalisedString::fromNumber(static_cast<int>(conns.size())));
	connectionsActive->setColour(conns.empty() ? inactiveCol : activeCol);
}

void RemotesStatusBar::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	if (button == 0) {
		projectWindow.setPage(EditorTabs::Remotes);
	}
}

std::optional<MouseCursorMode> RemotesStatusBar::getMouseCursorMode() const
{
	return MouseCursorMode::Hand;
}
