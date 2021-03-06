#include "graph_editor.h"

#include "src/ui/scroll_background.h"
using namespace Halley;

GraphEditor::GraphEditor(UIFactory& factory, Resources& resources, Project& project, AssetType type)
	: AssetEditor(factory, resources, project, type)
{
	factory.loadUI(*this, "ui/halley/graph_editor");
}

void GraphEditor::onMakeUI()
{
	scrollBg = getWidgetAs<ScrollBackground>("scrollBackground");
}
