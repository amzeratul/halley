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

	GraphEditor::reload();

	int i = 0;
	for (const auto& node: renderGraph->getNodes()) {
		auto nodeWidget = std::make_shared<UIRenderGraphNode>(*this, node, factory, factory.getStyle("graphNode"));
		addNode(nodeWidget);
		++i;
	}
}

std::shared_ptr<const Resource> RenderGraphEditor::loadResource(const String& assetId)
{
	return gameResources.get<RenderGraphDefinition>(assetId);
}

void RenderGraphEditor::drawConnections(UIPainter& painter)
{
	painter.draw([this] (Painter& painter)
	{
		for (const auto& connection: renderGraph->getConnections()) {
			const auto& fromNodeWidget = getNode(connection.fromId);
			const auto& toNodeWidget = getNode(connection.toId);
			const auto& fromNode = std::dynamic_pointer_cast<UIRenderGraphNode>(fromNodeWidget)->getNode();

			const auto outputPinWidget = fromNodeWidget->getPinWidget(true, connection.fromPin);
			const auto inputPinWidget = toNodeWidget->getPinWidget(false, connection.toPin);

			const auto startPos = outputPinWidget->getPosition() + outputPinWidget->getSize() / 2;
			const auto endPos = inputPinWidget->getPosition() + inputPinWidget->getSize() / 2;

			const auto fromPinType = fromNode.getOutputPins()[connection.fromPin];
			const auto col = getColourForPinType(fromPinType);

			drawConnection(painter, startPos, endPos, col);
		}
	});
}

void RenderGraphEditor::drawConnection(Painter& painter, Vector2f startPoint, Vector2f endPoint, Colour4f col) const
{
	const float dist = std::max(std::abs(endPoint.x - startPoint.x), 50.0f) / 2;
	const Vector2f p0 = startPoint;
	const Vector2f p1 = startPoint + Vector2f(dist, 0);
	const Vector2f p2 = endPoint - Vector2f(dist, 0);
	const Vector2f p3 = endPoint;
	
	std::vector<Vector2f> points;
	const size_t nPoints = 30; // eh

	const float scale = 1.0f / static_cast<float>(nPoints - 1);
	for (size_t i = 0; i < nPoints; ++i) {
		const float t = static_cast<float>(i) * scale;
		Vector2f p =
			(1 - t) * (1 - t) * (1 - t) * p0 +
			3 * (1 - t) * (1 - t) * t * p1 +
			3 * (1 - t) * t * t * p2 +
			t * t * t * p3;
		points.push_back(p);
	}
	painter.drawLine(points, 2, col);
}
