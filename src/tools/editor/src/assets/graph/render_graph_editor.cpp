#include "render_graph_editor.h"

#include "ui_graph_node.h"
using namespace Halley;

RenderGraphEditor::RenderGraphEditor(UIFactory& factory, Resources& gameResources, Project& project, AssetType type)
	: GraphEditor(factory, gameResources, project, type)
{
}

void RenderGraphEditor::reload()
{
	renderGraph = std::dynamic_pointer_cast<const RenderGraphDefinition>(resource);

	int i = 0;
	for (const auto& node: renderGraph->getNodes()) {
		addNode(node);
		++i;
	}
}

void RenderGraphEditor::drawConnections(UIPainter& painter)
{
	painter.draw([this] (Painter& painter)
	{
		for (const auto& connection: renderGraph->getConnections()) {
			const auto& fromNodeWidget = getNode(connection.fromId);
			const auto& toNodeWidget = getNode(connection.toId);
			const auto& fromNode = fromNodeWidget->getNode();
			const auto& toNode = toNodeWidget->getNode();

			const auto outputPinWidget = fromNodeWidget->getPinWidget(true, connection.fromPin);
			const auto inputPinWidget = toNodeWidget->getPinWidget(false, connection.toPin);

			const auto startPos = outputPinWidget->getPosition() + outputPinWidget->getSize() / 2;
			const auto endPos = inputPinWidget->getPosition() + inputPinWidget->getSize() / 2;

			const auto fromPinType = fromNode.getOutputPins()[connection.fromPin];
			const auto col = getColourForPinType(fromPinType);
			
			painter.drawLine({{ startPos, endPos }}, 2, col);
		}
	});
}

std::shared_ptr<const Resource> RenderGraphEditor::loadResource(const String& assetId)
{
	return gameResources.get<RenderGraphDefinition>(assetId);
}
