#include "assets_editor_window.h"
using namespace Halley;

AssetsEditorWindow::AssetsEditorWindow(UIFactory& factory, Project& project)
	: UIWidget("assets_editor", {}, UISizer())
	, factory(factory)
	, project(project)
{
	UIWidget::add(factory.makeUI("ui/halley/assets_editor_window"), 1);
}
