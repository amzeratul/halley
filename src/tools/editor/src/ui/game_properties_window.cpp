#include "game_properties_window.h"

#include "project_window.h"
#include "halley/tools/project/project.h"

namespace Halley {
	class ProjectWindow;
}

using namespace Halley;

GamePropertiesWindow::GamePropertiesWindow(UIFactory& factory, ProjectWindow& projectWindow)
	: UIWidget("game_properties_window", Vector2f(), UISizer())
	, projectWindow(projectWindow)
{
	factory.loadUI(*this, "halley/game_properties");
}

void GamePropertiesWindow::onMakeUI()
{
	auto cli = projectWindow.getSetting(EditorSettingType::Project, "commandLineArguments").asString("");

	bindData("commandLine", cli, [=] (String value)
	{
		projectWindow.setSetting(EditorSettingType::Project, "commandLineArguments", ConfigNode(value));
	});
}
