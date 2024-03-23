#include "halley/graph/base_graph_renderer.h"

#include "halley/graph/base_graph.h"
#include "halley/graphics/painter.h"
#include "halley/maths/colour.h"
#include "halley/scripting/script_node_type.h"
using namespace Halley;

bool BaseGraphRenderer::NodeUnderMouseInfo::operator==(const NodeUnderMouseInfo& other) const
{
	return nodeId == other.nodeId && element == other.element && elementId == other.elementId;
}

bool BaseGraphRenderer::NodeUnderMouseInfo::operator!=(const NodeUnderMouseInfo& other) const
{
	return !(*this == other);
}

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

std::tuple<Colour4f, Colour4f, float> BaseGraphRenderer::getNodeColour(const IGraphNodeType& nodeType, NodeDrawMode drawMode)
{
	const auto baseCol = getBaseNodeColour(nodeType);
	Colour4f col = baseCol;
	Colour4f iconCol = Colour4f(1, 1, 1);
	float borderAlpha = drawMode.selected ? 1.0f : 0.0f;
	
	switch (drawMode.type) {
	case NodeDrawModeType::Highlight:
		col = col.inverseMultiplyLuma(0.5f);
		break;
	case NodeDrawModeType::Active:
		{
			const float phase = drawMode.time * 2.0f * pif();
			col = col.inverseMultiplyLuma(sinRange(phase, 0.3f, 1.0f));
			borderAlpha = 1;
			break;
		}
	case NodeDrawModeType::Unvisited:
		col = col.multiplyLuma(0.3f);
		iconCol = Colour4f(0.5f, 0.5f, 0.5f);
		break;
	case NodeDrawModeType::Normal:
		break;
	}

	if (drawMode.activationTime < 1.0f) {
		const float t = drawMode.activationTime;
		const float t2 = std::pow(t, 0.5f);
		const float t3 = std::pow(t, 0.3f);
		const auto baseCol2 = lerp(baseCol, col, t2);
		iconCol = lerp(Colour4f(1, 1, 1), iconCol, t2);
		col = lerp(Colour4f(1, 1, 1), baseCol2, t3 * 0.5f + 0.5f);
	}

	return { col, iconCol, borderAlpha };
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

Colour4f BaseGraphRenderer::getBaseNodeColour(const IGraphNodeType& type) const
{
	return Colour4f(0.7f, 0.7f, 0.7f);
}

Vector2f BaseGraphRenderer::getNodeSize(const IGraphNodeType& nodeType, const BaseGraphNode& node, float curZoom) const
{
	if (auto size = nodeType.getNodeSize(node, curZoom)) {
		return size.value();
	} else {
		return Vector2f(64, 64);
	}
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

Circle BaseGraphRenderer::getNodeElementArea(const IGraphNodeType& nodeType, Vector2f basePos, const BaseGraphNode& node, size_t pinN, float curZoom, float posScale) const
{
	const Vector2f nodeSize = getNodeSize(nodeType, node, curZoom);
	const auto getOffset = [&] (size_t idx, size_t n, size_t axis)
	{
		const float spacing = nodeSize[axis] / (n + 1);
		return (static_cast<float>(idx) - (n - 1) * 0.5f) * spacing;
	};

	const auto& pin = nodeType.getPin(node, pinN);
	const auto pinSide = getSide(pin);
	
	size_t pinsOnSide = 0;
	size_t idxOnSide = 0;
	const auto& pins = nodeType.getPinConfiguration(node);
	for (size_t i = 0; i < pins.size(); ++i) {
		const auto& pinType = pins[i];
		if (i == pinN) {
			idxOnSide = pinsOnSide;
		}
		if (getSide(pinType) == pinSide) {
			++pinsOnSide;
		}
	}
	
	Vector2f offset;
	switch (pinSide) {
	case GraphPinSide::Left:
		offset = Vector2f(-nodeSize.x * 0.5f, getOffset(idxOnSide, pinsOnSide, 1));
		break;
	case GraphPinSide::Right:
		offset = Vector2f(nodeSize.x * 0.5f, getOffset(idxOnSide, pinsOnSide, 1));
		break;
	case GraphPinSide::Top:
		offset = Vector2f(getOffset(idxOnSide, pinsOnSide, 0), -nodeSize.y * 0.5f);
		break;
	case GraphPinSide::Bottom:
		offset = Vector2f(getOffset(idxOnSide, pinsOnSide, 0 ), nodeSize.y * 0.5f);
		break;
	default:
		break;
	}

	const Vector2f pos = basePos + node.getPosition() * posScale;
	const Vector2f centre = pos + offset / curZoom;
	const float radius = 4.0f / curZoom;
	
	return Circle(centre, radius);
}

std::optional<BaseGraphRenderer::NodeUnderMouseInfo> BaseGraphRenderer::getNodeUnderMouse(Vector2f basePos, float curZoom, Vector2f mousePos, bool pinPriority) const
{
	if (!graph) {
		return {};
	}

	const float effectiveZoom = std::max(nativeZoom, curZoom);

	float bestDistance = std::numeric_limits<float>::max();
	std::optional<NodeUnderMouseInfo> bestResult;
	
	for (size_t i = 0; i < graph->getNumNodes(); ++i) {
		const auto& node = graph->getNode(i);
		const auto pos = basePos + node.getPosition();

		const auto* nodeType = tryGetNodeType(node.getType());
		if (!nodeType) {
			continue;
		}
		const auto nodeSize = getNodeSize(*nodeType, node, effectiveZoom);
		const Rect4f area = Rect4f(-nodeSize / 2, nodeSize / 2) / effectiveZoom;
		const auto curRect = area + pos;

		if (!curRect.grow(10).contains(mousePos)) {
			continue;
		}
		
		// Check each pin handle
		bool foundPin = false;
		const auto& pins = node.getPinConfiguration();
		for	(size_t j = 0; j < pins.size(); ++j) {
			const auto& pinType = pins[j];
			const auto circle = getNodeElementArea(*nodeType, basePos, static_cast<const ScriptGraphNode&>(node), j, curZoom, 1.0f).expand((pinPriority ? 12.0f : 4.0f) / curZoom);
			if (circle.contains(mousePos)) {
				foundPin = true;
				const float distance = (mousePos - circle.getCentre()).length();
				if (distance < bestDistance) {
					bestDistance = distance;
					bestResult = NodeUnderMouseInfo{ static_cast<GraphNodeId>(i), pinType, static_cast<GraphPinId>(j), curRect, circle.getCentre() };
				}
			}
		}
		
		// Check main body
		if (!foundPin && curRect.contains(mousePos)) {
			const float distance = (mousePos - curRect.getCenter()).length();
			if (distance < bestDistance) {
				bestDistance = distance;
				bestResult = NodeUnderMouseInfo{ static_cast<GraphNodeId>(i), GraphNodePinType{ScriptNodeElementType::Node}, static_cast<GraphPinId>(-1), curRect, Vector2f() };
			}
		}
	}

	return bestResult;
}

BaseGraphRenderer::NodeUnderMouseInfo BaseGraphRenderer::getPinInfo(Vector2f basePos, float curZoom, GraphNodeId nodeId, GraphPinId pinId) const
{
	const auto& node = graph->getNode(nodeId);
	const auto& pins = node.getPinConfiguration();
	const auto& pinType = pins[pinId];
	const auto* nodeType = tryGetNodeType(node.getType());

	const auto pos = basePos + node.getPosition();
	const float effectiveZoom = std::max(nativeZoom, curZoom);
	const auto nodeSize = getNodeSize(*nodeType, node, effectiveZoom);
	const Rect4f area = Rect4f(-nodeSize / 2, nodeSize / 2) / effectiveZoom;
	const auto curRect = area + pos;

	const auto circle = getNodeElementArea(*nodeType, basePos, static_cast<const ScriptGraphNode&>(node), pinId, curZoom, 1.0f).expand(4.0f / curZoom);

	return NodeUnderMouseInfo{ nodeId, pinType, pinId, curRect, circle.getCentre() };
}

Vector2f BaseGraphRenderer::getPinPosition(Vector2f basePos, const BaseGraphNode& node, GraphPinId idx, float zoom) const
{
	return getNodeElementArea(static_cast<const ScriptGraphNode&>(node).getNodeType(), basePos, static_cast<const ScriptGraphNode&>(node), idx, zoom, 1.0f).getCentre();
}

Vector<GraphNodeId> BaseGraphRenderer::getNodesInRect(Vector2f basePos, float curZoom, Rect4f selBox) const
{
	if (!graph) {
		return {};
	}

	const float effectiveZoom = std::max(nativeZoom, curZoom);
	Vector<GraphNodeId> result;

	for (size_t i = 0; i < graph->getNumNodes(); ++i) {
		const auto& node = graph->getNode(i);
		const auto pos = basePos + node.getPosition();

		const auto* nodeType = tryGetNodeType(node.getType());
		if (!nodeType) {
			continue;
		}
		const auto nodeSize = getNodeSize(*nodeType, node, effectiveZoom);
		const Rect4f area = Rect4f(-nodeSize / 2, nodeSize / 2) / effectiveZoom;
		const auto curRect = area + pos;

		if (curRect.overlaps(selBox)) {
			result.push_back(static_cast<GraphNodeId>(i));
		}
	}

	return result;
}
