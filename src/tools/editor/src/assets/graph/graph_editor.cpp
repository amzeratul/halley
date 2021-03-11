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
	scrollBg->add(std::make_shared<GraphConnections>(*this));
}

std::shared_ptr<UIGraphNode> GraphEditor::getNode(std::string_view id)
{
	return getWidgetAs<UIGraphNode>(id);
}

void GraphEditor::drawConnections(UIPainter& painter)
{
}

void GraphEditor::addNode(const RenderGraphDefinition::Node& node)
{
	auto nodeWidget = std::make_shared<UIGraphNode>(*this, node, factory);
	scrollBg->add(std::move(nodeWidget), 0, {}, {}, node.position);
}

GraphConnections::GraphConnections(GraphEditor& editor)
	: UIWidget()
	, editor(editor)
{}

void GraphConnections::draw(UIPainter& painter) const
{
	editor.drawConnections(painter);
}

