#include "graph_editor.h"

#include "src/ui/scroll_background.h"
using namespace Halley;

GraphEditor::GraphEditor(UIFactory& factory, Resources& gameResources, Project& project, AssetType type)
	: AssetEditor(factory, gameResources, project, type)
{
	factory.loadUI(*this, "ui/halley/graph_editor");
}

void GraphEditor::onMakeUI()
{
	scrollBg = getWidgetAs<ScrollBackground>("scrollBackground");
}

void GraphEditor::addNode(Vector2f pos)
{
	auto sprite = factory.getStyle("input").getSprite("box");
	sprite.scaleTo(Vector2f(200, 100));
	auto nodeWidget = std::make_shared<UIImage>(sprite);
	scrollBg->add(nodeWidget, 0, {}, {}, pos);
}
