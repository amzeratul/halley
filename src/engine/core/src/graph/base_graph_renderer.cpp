#include "halley/graph/base_graph_renderer.h"

#include "halley/graph/base_graph.h"
#include "halley/graphics/painter.h"
#include "halley/maths/colour.h"
#include "halley/graph/base_graph_type.h"
#include "halley/resources/resources.h"
#include "halley/utils/algorithm.h"
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
	nodeBg = Sprite().setImage(resources, "halley_ui/ui_float_solid_window.png").setPivot(Vector2f(0.5f, 0.5f));
	nodeBgOutline = Sprite().setImage(resources, "halley_ui/ui_float_solid_window_outline.png").setPivot(Vector2f(0.5f, 0.5f));
	pinSprite = Sprite().setImage(resources, "halley_ui/ui_render_graph_node_pin.png").setPivot(Vector2f(0.5f, 0.5f));
	labelText
		.setFont(resources.get<Font>("Ubuntu Bold"))
		.setSize(14)
		.setColour(Colour(1, 1, 1))
		.setOutlineColour(Colour(0, 0, 0))
		.setOutline(1);
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

void BaseGraphRenderer::drawNodeOutputs(Painter& painter, Vector2f basePos, GraphNodeId nodeIdx, const BaseGraph& graph, float curZoom, float posScale) const
{
	const auto drawMode = getNodeDrawMode(nodeIdx);
	NodeDrawMode dstDrawMode;

	const BaseGraphNode& node = graph.getNode(nodeIdx);
	const auto* nodeType = tryGetNodeType(node.getType());
	if (!nodeType) {
		return;
	}
	const bool nodeHighlighted = highlightNode && highlightNode->nodeId == nodeIdx && highlightNode->element.type == static_cast<GraphElementType>(BaseGraphNodeElementType::Node);

	for (size_t i = 0; i < node.getPins().size(); ++i) {
		const auto& srcPinType = nodeType->getPin(node, i);
		const auto& pin = node.getPins()[i];

		const bool pinHighlighted = nodeHighlighted || (highlightNode && highlightNode->nodeId == nodeIdx && highlightNode->elementId == i);
		
		for (const auto& pinConnection: pin.connections) {
			std::optional<Vector2f> dstPos;
			GraphNodePinType dstPinType;

			bool highlighted = pinHighlighted;

			if (pinConnection.dstNode && srcPinType.direction == GraphNodePinDirection::Output) {
				const size_t dstIdx = pinConnection.dstPin;
				const auto& dstNode = graph.getNode(pinConnection.dstNode.value());
				const auto* dstNodeType = tryGetNodeType(dstNode.getType());
				if (!dstNodeType) {
					continue;
				}
				dstPos = getNodeElementArea(*dstNodeType, basePos, dstNode, dstIdx, curZoom, posScale).getCentre();
				dstPinType = dstNodeType->getPin(dstNode, dstIdx);
				if (highlightNode && highlightNode->nodeId == pinConnection.dstNode.value()) {
					if (highlightNode->element.type == static_cast<GraphElementType>(BaseGraphNodeElementType::Node) || highlightNode->elementId == pinConnection.dstPin) {
						highlighted = true;
					}
				}

				dstDrawMode = getNodeDrawMode(*pinConnection.dstNode);
			}
			
			if (dstPos) {
				const Vector2f srcPos = getNodeElementArea(*nodeType, basePos, node, i, curZoom, posScale).getCentre();
				const bool connActive = drawMode.type != NodeDrawModeType::Unvisited && dstDrawMode.type != NodeDrawModeType::Unvisited;
				drawConnection(painter, ConnectionPath{ srcPos, dstPos.value(), srcPinType, dstPinType }, curZoom, highlighted, !connActive);
			}
		}
	}
}

void BaseGraphRenderer::drawNodeBackground(Painter& painter, Vector2f basePos, const BaseGraphNode& node, float curZoom, float posScale, NodeDrawMode drawMode)
{
}

void BaseGraphRenderer::drawNode(Painter& painter, Vector2f basePos, const BaseGraphNode& node, float curZoom, float posScale, NodeDrawMode drawMode, std::optional<GraphNodePinType> highlightElement, GraphPinId highlightElementId)
{
	const auto* nodeType = tryGetNodeType(node.getType());
	if (!nodeType) {
		return;
	}

	const Vector2f border = Vector2f(18, 18);
	const Vector2f nodeSize = getNodeSize(*nodeType, node, curZoom);
	const auto pos = ((basePos + node.getPosition() * posScale) * curZoom).round() / curZoom;
	const auto [c, iconCol, borderAlpha] = getNodeColour(*nodeType, drawMode);
	auto col = c; // Clang doesn't seem to like lambda capturing (drawLabel, below) from a structured binding

	// Node body
	nodeBg.clone()
		.setColour(col)
		.setPosition(pos)
		.scaleTo(nodeSize + border)
		.setSize(nodeBg.getSize() / curZoom)
		.setSliceScale(1.0f / curZoom)
		.draw(painter);

	if (borderAlpha > 0.0001f) {
		nodeBgOutline.clone()
			.setPosition(pos)
			.scaleTo(nodeSize + border)
			.setSize(nodeBg.getSize() / curZoom)
			.setSliceScale(1.0f / curZoom)
			.setColour(Colour4f(1, 1, 1, borderAlpha))
			.draw(painter);
	}

	auto [label, labelType] = getLabel(*tryGetNodeType(node.getType()), node);
	const Vector2f iconOffset = labelType == LabelType::Normal ? Vector2f(0, -8.0f / curZoom).round() : Vector2f();

	// Icon
	const auto& icon = getIconByName(nodeType->getIconName(node));
	const float iconAlpha = getIconAlpha(*nodeType, labelType == LabelType::Large);
	if (icon.hasMaterial() && iconAlpha > 0.0f) {
		icon.clone()
			.setPosition(pos + iconOffset)
			.setScale(1.0f / curZoom)
			.setColour(iconCol.multiplyAlpha(iconAlpha))
			.draw(painter);
	}

	auto drawLabel = [&, col](const String& text, Vector2f pos, float size, float maxWidth, bool split)
	{
		auto labelCopy = labelText.clone()
			.setPosition(pos)
			.setSize(size)
			.setOutline(8.0f / curZoom)
			.setOutlineColour(col.multiplyLuma(0.75f))
			.setOffset(Vector2f(0.5f, 0.5f))
			.setAlignment(0.0f);

		if (split) {
			labelCopy.setText(labelCopy.split(text, maxWidth));
		} else {
			labelCopy.setText(text);
			const auto extents = labelCopy.getExtents();
			if (extents.x > maxWidth) {
				labelCopy.setSize(size * maxWidth / extents.x);
			}
		}

		labelCopy
			.draw(painter);
	};

	// Large label
	switch (labelType) {
	case LabelType::Normal:
		drawLabel(label, pos + Vector2f(0, 18.0f / curZoom).round(), 14.0f / curZoom, (nodeSize.x - 10.0f) / curZoom, false);
		break;
	case LabelType::Large:
		drawLabel(label, pos + iconOffset, 18.0f / curZoom, (nodeSize.x - 10.0f) / curZoom, false);
		break;
	case LabelType::Comment:
		drawLabel(label, pos + iconOffset, 14.0f / curZoom, (nodeSize.x - 20.0f) / curZoom, true);
		break;
	}

	// Draw pins
	const auto& pins = nodeType->getPinConfiguration(node);
	for (size_t i = 0; i < pins.size(); ++i) {
		const auto& pinType = pins[i];
		const auto circle = getNodeElementArea(*nodeType, basePos, node, i, curZoom, posScale);
		const auto baseCol = getPinColour(pinType);
		const auto col = highlightElement == pinType && highlightElementId == i ? baseCol.inverseMultiplyLuma(0.3f) : baseCol;
		pinSprite.clone()
			.setPosition(circle.getCentre())
			.setColour(col)
			.setScale(1.0f / curZoom)
			.draw(painter);
	}
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
	NodeDrawMode drawMode;
	const bool highlightThis = highlightNode && highlightNode->nodeId == nodeId;
	if (highlightThis && highlightNode->element.type == static_cast<GraphElementType>(BaseGraphNodeElementType::Node)) {
		drawMode.type = NodeDrawModeType::Highlight;
	}
	drawMode.selected = std_ex::contains(selectedNodes, nodeId);
	return drawMode;
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
			const auto circle = getNodeElementArea(*nodeType, basePos, node, j, curZoom, 1.0f).expand((pinPriority ? 12.0f : 4.0f) / curZoom);
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
				bestResult = NodeUnderMouseInfo{ static_cast<GraphNodeId>(i), GraphNodePinType{BaseGraphNodeElementType::Node}, static_cast<GraphPinId>(-1), curRect, Vector2f() };
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

	const auto circle = getNodeElementArea(*nodeType, basePos, node, pinId, curZoom, 1.0f).expand(4.0f / curZoom);

	return NodeUnderMouseInfo{ nodeId, pinType, pinId, curRect, circle.getCentre() };
}

Vector2f BaseGraphRenderer::getPinPosition(Vector2f basePos, const BaseGraphNode& node, GraphPinId idx, float zoom) const
{
	return getNodeElementArea(*tryGetNodeType(node.getType()), basePos, node, idx, zoom, 1.0f).getCentre();
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

float BaseGraphRenderer::getIconAlpha(const IGraphNodeType& graphNode, bool dim) const
{
	return dim ? 0.25f : 1.0f;
}

std::pair<String, BaseGraphRenderer::LabelType> BaseGraphRenderer::getLabel(const IGraphNodeType& nodeType, const BaseGraphNode& node) const
{
	const auto label = nodeType.getLabel(node);
	if (!label.isEmpty()) {
		return { label, LabelType::Normal };
	}

	return { "", LabelType::None };
}
