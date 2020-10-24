#include "asset_editor_window.h"
using namespace Halley;

AssetEditorWindow::AssetEditorWindow(UIFactory& factory)
	: UIWidget("asset_editor_window", Vector2f(), UISizer())
{
	factory.loadUI(*this, "ui/halley/asset_editor_window");
}
