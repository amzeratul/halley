#include "halley/graph/base_graph_renderer.h"

#include "halley/graph/base_graph.h"
#include "halley/graphics/painter.h"
#include "halley/maths/colour.h"
using namespace Halley;

BaseGraphRenderer::BaseGraphRenderer(Resources& resources, float nativeZoom)
	: resources(resources)
	, nativeZoom(nativeZoom)
{
}

void BaseGraphRenderer::setGraph(const BaseGraph* graph)
{
	this->graph = graph;
}

void BaseGraphRenderer::draw(Painter& painter, Vector2f basePos, float curZoom, float posScale)
{
	if (!graph) {
		return;
	}

	const float effectiveZoom = std::max(nativeZoom, curZoom);

	for (GraphNodeId i = 0; i < static_cast<GraphNodeId>(graph->getNumNodes()); ++i) {
		if (graph->getNode(i).canDraw()) {
			drawNodeBackground(painter, basePos, graph->getNode(i), effectiveZoom, posScale, getNodeDrawMode(i));
		}
	}

	for (size_t i = 0; i < graph->getNumNodes(); ++i) {
		if (graph->getNode(i).canDraw()) {
			drawNodeOutputs(painter, basePos, static_cast<GraphNodeId>(i), *graph, effectiveZoom, posScale);
		}
	}

	for (const auto& currentPath: currentPaths) {
		drawConnection(painter, currentPath, curZoom, false, currentPath.fade);
	}
	
	for (GraphNodeId i = 0; i < static_cast<GraphNodeId>(graph->getNumNodes()); ++i) {
		if (graph->getNode(i).canDraw()) {
			const bool highlightThis = highlightNode && highlightNode->nodeId == i;
			auto pinType = highlightThis ? std::optional<GraphNodePinType>(highlightNode->element) : std::optional<GraphNodePinType>();
			auto pinId = highlightThis ? highlightNode->elementId : 0;

			if (highlightNode && !highlightThis) {
				const auto& pin = graph->getNode(highlightNode->nodeId).getPin(highlightNode->elementId);
				for (const auto& conn: pin.connections) {
					if (conn.dstNode == i) {
						pinId = conn.dstPin;
						pinType = graph->getNode(i).getPinType(pinId);
					}
				}
			}
			
			drawNode(painter, basePos, graph->getNode(i), effectiveZoom, posScale, getNodeDrawMode(i), pinType, pinId);
		}
	}
}

void BaseGraphRenderer::setHighlight(std::optional<NodeUnderMouseInfo> node)
{
	highlightNode = std::move(node);
}

void BaseGraphRenderer::setSelection(Vector<GraphNodeId> nodes)
{
	selectedNodes = std::move(nodes);
}

void BaseGraphRenderer::setCurrentPaths(Vector<ConnectionPath> path)
{
	currentPaths = std::move(path);
}

void BaseGraphRenderer::setDebugDisplayData(HashMap<int, String> values)
{
}

void BaseGraphRenderer::drawConnection(Painter& painter, const ConnectionPath& path, float curZoom, bool highlight, bool fade) const
{
	const bool dimmed = isDimmed(path.fromType);
	const auto bezier = makeBezier(path);
	const auto baseCol = getPinColour(path.fromType);
	Colour4f normalCol = baseCol;
	Colour4f highlightCol = baseCol;
	float shadowAlpha = 0.3f;

	if (dimmed) {
		normalCol = normalCol.multiplyLuma(0.4f);
		shadowAlpha = 0.15f;
	} else {
		highlightCol = highlightCol.inverseMultiplyLuma(0.25f);
	}
	const auto col = (highlight ? highlightCol : normalCol).multiplyAlpha(fade ? 0.5f : 1.0f);

	Painter::LineParameters pattern;
	if (path.fromType.isDetached) {
		pattern.onLength = 8.0f;
		pattern.offLength = 8.0f;
	}

	painter.drawLine(bezier + Vector2f(1.0f, 2.0f) / curZoom, 3.0f / curZoom, Colour4f(0, 0, 0, shadowAlpha), {}, pattern);
	painter.drawLine(bezier, 3.0f / curZoom, col, {}, pattern);
}

void BaseGraphRenderer::drawNodeOutputs(Painter& painter, Vector2f basePos, GraphNodeId nodeIdx, const BaseGraph& graph, float curZoom, float posScale)
{
}

void BaseGraphRenderer::drawNodeBackground(Painter& painter, Vector2f basePos, const BaseGraphNode& node, float curZoom, float posScale, NodeDrawMode drawMode)
{
}

void BaseGraphRenderer::drawNode(Painter& painter, Vector2f basePos, const BaseGraphNode& node, float curZoom, float posScale, NodeDrawMode drawMode, std::optional<GraphNodePinType> highlightElement, GraphPinId highlightElementId)
{
}

BaseGraphRenderer::NodeDrawMode BaseGraphRenderer::getNodeDrawMode(GraphNodeId nodeId) const
{
	return {};
}

bool BaseGraphRenderer::isDimmed(GraphNodePinType type) const
{
	return false;
}

GraphPinSide BaseGraphRenderer::getSide(GraphNodePinType pinType) const
{
	return pinType.direction == GraphNodePinDirection::Input ? GraphPinSide::Left : GraphPinSide::Right;
}

Colour4f BaseGraphRenderer::getPinColour(GraphNodePinType pinType) const
{
	return Colour4f(1, 1, 1, 1);
}

const Sprite& BaseGraphRenderer::getIconByName(const String& iconName)
{
	const auto iter = icons.find(iconName);
	if (iter != icons.end()) {
		return iter->second;
	}
	icons[iconName] = iconName.isEmpty() ? Sprite() : Sprite().setImage(resources, iconName).setPivot(Vector2f(0.5f, 0.5f));
	return icons[iconName];
}

BezierCubic BaseGraphRenderer::makeBezier(const ConnectionPath& path) const
{
	auto getSideNormal = [] (GraphPinSide side) -> Vector2f
	{
		switch (side) {
		case GraphPinSide::Left:
			return Vector2f(-1, 0);
		case GraphPinSide::Right:
			return Vector2f(1, 0);
		case GraphPinSide::Top:
			return Vector2f(0, -1);
		case GraphPinSide::Bottom:
			return Vector2f(0, 1);
		}
		return Vector2f();
	};
	
	const Vector2f fromDir = getSideNormal(getSide(path.fromType));
	const Vector2f toDir = getSideNormal(getSide(path.toType));

	const auto delta = path.to - path.from;
	const float dist = std::max(std::max(std::abs(delta.x), std::abs(delta.y)), 20.0f) / 2;

	return BezierCubic(path.from, path.from + dist * fromDir, path.to + dist * toDir, path.to);
}
