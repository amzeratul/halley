#include "editor_settings_window.h"
using namespace Halley;

EditorSettingsWindow::EditorSettingsWindow(UIFactory& factory, Preferences& preferences, Project& project, ProjectLoader& projectLoader)
	: UIWidget("editor_settings_window", Vector2f(), UISizer())
	, preferences(preferences)
	, project(project)
	, projectLoader(projectLoader)
{
	factory.loadUI(*this, "ui/halley/editor_settings");
}

void EditorSettingsWindow::onMakeUI()
{
	
}
