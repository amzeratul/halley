#include "editor_settings_window.h"
using namespace Halley;

EditorSettingsWindow::EditorSettingsWindow(UIFactory& factory)
	: UIWidget("editor_settings_window", Vector2f(), UISizer())
{
	factory.loadUI(*this, "ui/halley/editor_settings");
}

void EditorSettingsWindow::onMakeUI()
{
	
}
