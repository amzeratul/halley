#include "editor_settings_window.h"
using namespace Halley;

EditorSettingsWindow::EditorSettingsWindow(UIFactory& factory, Preferences& preferences)
	: UIWidget("editor_settings_window", Vector2f(), UISizer())
	, preferences(preferences)
{
	factory.loadUI(*this, "ui/halley/editor_settings");
}

void EditorSettingsWindow::onMakeUI()
{
	
}
