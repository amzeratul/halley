#include "graph_editor.h"


#include "ui_graph_node.h"
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
	scrollBg->setZoomEnabled(false);
}

void GraphEditor::addNode(const RenderGraphDefinition::Node& node)
{
	auto nodeWidget = std::make_shared<UIGraphNode>(node, factory);
	scrollBg->add(std::move(nodeWidget), 0, {}, {}, node.position);
}
