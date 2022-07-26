#include "scripting/script_renderer.h"
#include "world.h"
#include "halley/core/graphics/painter.h"
#include "halley/maths/bezier.h"
#include "halley/support/logger.h"
#include "halley/utils/algorithm.h"
#include "scripting/script_graph.h"
#include "scripting/script_node_type.h"
using namespace Halley;

#ifndef DONT_INCLUDE_HALLEY_HPP
#define DONT_INCLUDE_HALLEY_HPP
#endif
#include "components/transform_2d_component.h"

bool ScriptRenderer::NodeUnderMouseInfo::operator==(const NodeUnderMouseInfo& other) const
{
	return nodeId == other.nodeId && element == other.element && elementId == other.elementId;
}

bool ScriptRenderer::NodeUnderMouseInfo::operator!=(const NodeUnderMouseInfo& other) const
{
	return !(*this == other);
}

ScriptRenderer::ScriptRenderer(Resources& resources, const World* world, const ScriptNodeTypeCollection& nodeTypeCollection, float nativeZoom)
	: resources(resources)
	, world(world)
	, nodeTypeCollection(nodeTypeCollection)
	, nativeZoom(nativeZoom)
{
	nodeBg = Sprite().setImage(resources, "halley_ui/ui_float_solid_window.png").setPivot(Vector2f(0.5f, 0.5f));
	nodeBgOutline = Sprite().setImage(resources, "halley_ui/ui_float_solid_window_outline.png").setPivot(Vector2f(0.5f, 0.5f));
	variableBg = Sprite().setImage(resources, "halley_ui/script_variable.png").setPivot(Vector2f(0.5f, 0.5f));
	variableBgOutline = Sprite().setImage(resources, "halley_ui/script_variable_outline.png").setPivot(Vector2f(0.5f, 0.5f));
	destructorBg = Sprite().setImage(resources, "halley_ui/script_destructor_bg.png").setPivot(Vector2f(0.5f, 0.0f));
	destructorIcon = Sprite().setImage(resources, "halley_ui/script_destructor_icon.png").setPivot(Vector2f(0.5f, 0.5f));
	pinSprite = Sprite().setImage(resources, "halley_ui/ui_render_graph_node_pin.png").setPivot(Vector2f(0.5f, 0.5f));
	labelText
		.setFont(resources.get<Font>("Ubuntu Bold"))
		.setSize(14)
		.setColour(Colour(1, 1, 1))
		.setOutlineColour(Colour(0, 0, 0))
		.setOutline(1)
		.setAlignment(0.5f);
}

void ScriptRenderer::setGraph(const ScriptGraph* graph)
{
	this->graph = graph;
}

void ScriptRenderer::setState(const ScriptState* scriptState)
{
	state = scriptState;
}

void ScriptRenderer::draw(Painter& painter, Vector2f basePos, float curZoom, float posScale)
{
	if (!graph) {
		return;
	}

	const float effectiveZoom = std::max(nativeZoom, curZoom);

	for (size_t i = 0; i < graph->getNodes().size(); ++i) {
		drawNodeOutputs(painter, basePos, static_cast<ScriptNodeId>(i), *graph, effectiveZoom, posScale);
	}

	for (const auto& currentPath: currentPaths) {
		drawConnection(painter, currentPath, curZoom, false, currentPath.fade);
	}
	
	for (ScriptNodeId i = 0; i < static_cast<ScriptNodeId>(graph->getNodes().size()); ++i) {
		const bool highlightThis = highlightNode && highlightNode->nodeId == i;
		auto pinType = highlightThis ? highlightNode->element : std::optional<ScriptNodePinType>();
		auto pinId = highlightThis ? highlightNode->elementId : 0;

		if (highlightNode && !highlightThis) {
			const auto& pin = graph->getNodes()[highlightNode->nodeId].getPin(highlightNode->elementId);
			for (const auto& conn: pin.connections) {
				if (conn.dstNode == i) {
					pinId = conn.dstPin;
					pinType = graph->getNodes()[i].getPinType(pinId);
				}
			}
		}
		
		drawNode(painter, basePos, graph->getNodes()[i], effectiveZoom, posScale, getNodeDrawMode(i), pinType, pinId);
	}
}

void ScriptRenderer::drawNodeOutputs(Painter& painter, Vector2f basePos, ScriptNodeId nodeIdx, const ScriptGraph& graph, float curZoom, float posScale)
{
	auto drawMode = getNodeDrawMode(nodeIdx);
	NodeDrawMode dstDrawMode;

	const ScriptGraphNode& node = graph.getNodes().at(nodeIdx);
	const auto* nodeType = nodeTypeCollection.tryGetNodeType(node.getType());
	if (!nodeType) {
		return;
	}
	const bool nodeHighlighted = highlightNode && highlightNode->nodeId == nodeIdx && highlightNode->element.type == ScriptNodeElementType::Node;

	for (size_t i = 0; i < node.getPins().size(); ++i) {
		const auto& srcPinType = nodeType->getPin(node, i);
		const auto& pin = node.getPins()[i];

		const bool pinHighlighted = nodeHighlighted || (highlightNode && highlightNode->nodeId == nodeIdx && highlightNode->elementId == i);
		
		for (const auto& pinConnection: pin.connections) {
			std::optional<Vector2f> dstPos;
			ScriptNodePinType dstPinType;

			bool highlighted = pinHighlighted;

			if (pinConnection.dstNode && srcPinType.direction == ScriptNodePinDirection::Output) {
				const size_t dstIdx = pinConnection.dstPin;
				const auto& dstNode = graph.getNodes().at(pinConnection.dstNode.value());
				const auto* dstNodeType = nodeTypeCollection.tryGetNodeType(dstNode.getType());
				if (!dstNodeType) {
					continue;
				}
				dstPos = getNodeElementArea(*dstNodeType, basePos, dstNode, dstIdx, curZoom, posScale).getCentre();
				dstPinType = dstNodeType->getPin(dstNode, dstIdx);
				if (highlightNode && highlightNode->nodeId == pinConnection.dstNode.value()) {
					if (highlightNode->element.type == ScriptNodeElementType::Node || highlightNode->elementId == pinConnection.dstPin) {
						highlighted = true;
					}
				}

				dstDrawMode = getNodeDrawMode(*pinConnection.dstNode);
			} else if (pinConnection.entityIdx && world) {
				const auto entityId = graph.getEntityId(pinConnection.entityIdx);
				auto entity = world->tryGetEntity(entityId);
				if (entity.isValid()) {
					const auto* transform = entity.tryGetComponent<Transform2DComponent>();
					if (transform) {
						dstPos = transform->getGlobalPosition();
						dstPinType = ScriptNodePinType{ ScriptNodeElementType::TargetPin, ScriptNodePinDirection::Output };
					}
				}

				if (highlightEntity == pinConnection.entityIdx) {
					highlighted = true;
				}
			}
			
			if (dstPos) {
				const Vector2f srcPos = getNodeElementArea(*nodeType, basePos, node, i, curZoom, posScale).getCentre();
				const bool connActive = drawMode.type != NodeDrawModeType::Unvisited && dstDrawMode.type != NodeDrawModeType::Unvisited;
				drawConnection(painter, ConnectionPath{ srcPos, dstPos.value(), srcPinType, dstPinType }, curZoom, highlighted, !connActive);
			}
		}
	}
}

BezierCubic ScriptRenderer::makeBezier(const ConnectionPath& path) const
{
	auto getSideNormal = [] (ScriptPinSide side) -> Vector2f
	{
		switch (side) {
		case ScriptPinSide::Left:
			return Vector2f(-1, 0);
		case ScriptPinSide::Right:
			return Vector2f(1, 0);
		case ScriptPinSide::Top:
			return Vector2f(0, -1);
		case ScriptPinSide::Bottom:
			return Vector2f(0, 1);
		}
		return Vector2f();
	};
	
	const Vector2f fromDir = getSideNormal(path.fromType.getSide());
	const Vector2f toDir = getSideNormal(path.toType.getSide());

	const auto delta = path.to - path.from;
	const float dist = std::max(std::max(std::abs(delta.x), std::abs(delta.y)), 20.0f) / 2;

	return BezierCubic(path.from, path.from + dist * fromDir, path.to + dist * toDir, path.to);
}

void ScriptRenderer::drawConnection(Painter& painter, const ConnectionPath& path, float curZoom, bool highlight, bool fade) const
{
	const auto bezier = makeBezier(path);
	const auto baseCol = getPinColour(path.fromType);
	const auto col = (highlight ? baseCol.inverseMultiplyLuma(0.25f) : baseCol).multiplyAlpha(fade ? 0.5f : 1.0f);
	painter.drawLine(bezier + Vector2f(1.0f, 2.0f) / curZoom, 3.0f / curZoom, Colour4f(0, 0, 0, 0.3f));
	painter.drawLine(bezier, 3.0f / curZoom, col);
}

ScriptRenderer::NodeDrawMode ScriptRenderer::getNodeDrawMode(ScriptNodeId nodeId) const
{
	NodeDrawMode drawMode;
	if (state) {
		// Rendering in-game, with execution state
		const auto nodeIntrospection = state->getNodeIntrospection(nodeId);
		drawMode.activationTime = nodeIntrospection.activationTime < 0.5f ? nodeIntrospection.activationTime * 2 : std::numeric_limits<float>::infinity();
		if (nodeIntrospection.state == ScriptState::NodeIntrospectionState::Active) {
			drawMode.type = NodeDrawModeType::Active;
			drawMode.time = nodeIntrospection.time;
		} else if (nodeIntrospection.state == ScriptState::NodeIntrospectionState::Unvisited) {
			drawMode.type = NodeDrawModeType::Unvisited;
		}
	} else {
		// Rendering in editor
		const bool highlightThis = highlightNode && highlightNode->nodeId == nodeId;
		if (highlightThis && highlightNode->element.type == ScriptNodeElementType::Node) {
			drawMode.type = NodeDrawModeType::Highlight;
		}
	}
	drawMode.selected = std_ex::contains(selectedNodes, nodeId);
	return drawMode;
}

void ScriptRenderer::drawNode(Painter& painter, Vector2f basePos, const ScriptGraphNode& node, float curZoom, float posScale, NodeDrawMode drawMode, std::optional<ScriptNodePinType> highlightElement, ScriptPinId highlightElementId)
{
	const auto* nodeType = nodeTypeCollection.tryGetNodeType(node.getType());
	if (!nodeType) {
		return;
	}

	const Vector2f border = Vector2f(18, 18);
	const Vector2f nodeSize = getNodeSize(*nodeType, curZoom);
	const auto pos = ((basePos + node.getPosition() * posScale) * curZoom).round() / curZoom;

	{
		const auto baseCol = getNodeColour(*nodeType);
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
		}

		if (drawMode.activationTime < 1.0f) {
			const float t = drawMode.activationTime;
			const float t2 = std::pow(t, 0.5f);
			const float t3 = std::pow(t, 0.3f);
			const auto baseCol2 = lerp(baseCol, col, t2);
			iconCol = lerp(Colour4f(1, 1, 1), iconCol, t2);
			col = lerp(Colour4f(1, 1, 1), baseCol2, t3 * 0.5f + 0.5f);
		}

		// Destructor
		if (nodeType->hasDestructor(node) && nodeType->showDestructor()) {
			destructorBg.clone()
				.setColour(col.multiplyLuma(0.6f))
				.setPosition(pos + Vector2f(0, 29) / curZoom)
				.setScale(1.0f / curZoom)
				.draw(painter);

			destructorIcon.clone()
				.setPosition(pos + Vector2f(0, 36) / curZoom)
				.setScale(1.0f / curZoom)
				.draw(painter);
		}
		
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

		const auto label = nodeType->getLabel(node);
		const auto largeLabel = nodeType->getLargeLabel(node);
		const Vector2f iconOffset = label.isEmpty() ? Vector2f() : Vector2f(0, -8.0f / curZoom).round();

		auto drawLabel = [&](const String& text, Vector2f pos, float size, float maxWidth)
		{
			auto labelCopy = labelText.clone()
				.setPosition(pos)
				.setText(text)
				.setSize(size)
				.setOutline(8.0f / curZoom)
				.setOutlineColour(col.multiplyLuma(0.75f))
				.setOffset(Vector2f(0, 0.5f));
			
			const auto extents = labelCopy.getExtents();
			if (extents.x > maxWidth) {
				labelCopy.setSize(size * maxWidth / extents.x);
			}

			labelCopy
				.draw(painter);
		};
		
		// Icon
		const auto& icon = getIcon(*nodeType, node);
		if (icon.hasMaterial()) {
			icon.clone()
				.setPosition(pos + iconOffset)
				.setScale(1.0f / curZoom)
				.setColour(iconCol.multiplyAlpha(largeLabel.isEmpty() ? 1.0f : 0.25f))
				.draw(painter);
		}

		// Large label
		if (!largeLabel.isEmpty()) {
			drawLabel(largeLabel, pos + iconOffset, 18 / curZoom, (nodeSize.x - 10.0f) / curZoom);
		}

		// Label
		if (!label.isEmpty()) {
			drawLabel(label, pos + Vector2f(0, 18.0f / curZoom).round(), 14 / curZoom, (nodeSize.x - 10.0f) / curZoom);
		}
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

Vector2f ScriptRenderer::getNodeSize(const IScriptNodeType& nodeType, float curZoom) const
{
	return nodeType.getClassification() == ScriptNodeClassification::Variable ? Vector2f(100, 40) : Vector2f(60, 60);
}

Circle ScriptRenderer::getNodeElementArea(const IScriptNodeType& nodeType, Vector2f basePos, const ScriptGraphNode& node, size_t pinN, float curZoom, float posScale) const
{
	const Vector2f nodeSize = getNodeSize(nodeType, curZoom);
	const auto getOffset = [&] (size_t idx, size_t n)
	{
		const float spacing = nodeSize.x / (n + 1);
		return (static_cast<float>(idx) - (n - 1) * 0.5f) * spacing;
	};

	const auto& pin = nodeType.getPin(node, pinN);
	const auto pinSide = pin.getSide();
	
	size_t pinsOnSide = 0;
	size_t idxOnSide = 0;
	const auto& pins = nodeType.getPinConfiguration(node);
	for (size_t i = 0; i < pins.size(); ++i) {
		const auto& pinType = pins[i];
		if (i == pinN) {
			idxOnSide = pinsOnSide;
		}
		if (pinType.getSide() == pinSide) {
			++pinsOnSide;
		}
	}
	
	const auto sideOffset = getOffset(idxOnSide, pinsOnSide);
	Vector2f offset;
	switch (pinSide) {
	case ScriptPinSide::Left:
		offset = Vector2f(-nodeSize.x * 0.5f, sideOffset);
		break;
	case ScriptPinSide::Right:
		offset = Vector2f(nodeSize.x * 0.5f, sideOffset);
		break;
	case ScriptPinSide::Top:
		offset = Vector2f(sideOffset, -nodeSize.y * 0.5f);
		break;
	case ScriptPinSide::Bottom:
		offset = Vector2f(sideOffset, nodeSize.y * 0.5f);
		break;
	default:
		break;
	}

	const Vector2f pos = basePos + node.getPosition() * posScale;
	const Vector2f centre = pos + offset / curZoom;
	const float radius = 4.0f / curZoom;
	
	return Circle(centre, radius);
}

Colour4f ScriptRenderer::getNodeColour(const IScriptNodeType& nodeType)
{
	switch (nodeType.getClassification()) {
	case ScriptNodeClassification::Terminator:
		return Colour4f(0.97f, 0.35f, 0.35f);
	case ScriptNodeClassification::Action:
		return Colour4f(0.07f, 0.84f, 0.09f);
	case ScriptNodeClassification::Variable:
	case ScriptNodeClassification::Expression:
		return Colour4f(0.91f, 0.71f, 0.0f);
	case ScriptNodeClassification::FlowControl:
		return Colour4f(0.35f, 0.55f, 0.97f);
	case ScriptNodeClassification::State:
		return Colour4f(0.75f, 0.35f, 0.97f);
	case ScriptNodeClassification::Function:
		return Colour4f(1.00f, 0.49f, 0.68f);
	}
	return Colour4f(0.2f, 0.2f, 0.2f);
}

Colour4f ScriptRenderer::getPinColour(ScriptNodePinType pinType) const
{
	switch (pinType.type) {
	case ScriptNodeElementType::FlowPin:
		return pinType.isCancellable ? Colour4f(0.75f, 0.0f, 0.99f) : Colour4f(0.75f, 0.75f, 0.99f);
	case ScriptNodeElementType::ReadDataPin:
		return Colour4f(0.91f, 0.55f, 0.2f);
	case ScriptNodeElementType::WriteDataPin:
		return Colour4f(0.91f, 0.2f, 0.2f);
	case ScriptNodeElementType::TargetPin:
		return Colour4f(0.35f, 1, 0.35f);
	}

	return Colour4f();
}

const Sprite& ScriptRenderer::getIcon(const IScriptNodeType& nodeType, const ScriptGraphNode& node)
{
	const auto& iconName = nodeType.getIconName(node);
	
	const auto iter = icons.find(iconName);
	if (iter != icons.end()) {
		return iter->second;
	}
	icons[iconName] = iconName.isEmpty() ? Sprite() : Sprite().setImage(resources, iconName).setPivot(Vector2f(0.5f, 0.5f));
	return icons[iconName];
}

std::optional<ScriptRenderer::NodeUnderMouseInfo> ScriptRenderer::getNodeUnderMouse(Vector2f basePos, float curZoom, Vector2f mousePos, bool pinPriority) const
{
	if (!graph) {
		return {};
	}

	const float effectiveZoom = std::max(nativeZoom, curZoom);

	float bestDistance = std::numeric_limits<float>::max();
	std::optional<NodeUnderMouseInfo> bestResult;
	
	for (size_t i = 0; i < graph->getNodes().size(); ++i) {
		const auto& node = graph->getNodes()[i];
		const auto pos = basePos + node.getPosition();

		const auto nodeBounds = Circle(pos, 60.0f);
		if (!nodeBounds.contains(mousePos)) {
			continue;
		}
		
		const auto* nodeType = nodeTypeCollection.tryGetNodeType(node.getType());
		if (!nodeType) {
			continue;
		}
		const auto nodeSize = getNodeSize(*nodeType, effectiveZoom);
		const Rect4f area = Rect4f(-nodeSize / 2, nodeSize / 2) / effectiveZoom;
		const auto curRect = area + pos;
		
		// Check each pin handle
		bool foundPin = false;
		const auto& pins = nodeType->getPinConfiguration(node);
		for	(size_t j = 0; j < pins.size(); ++j) {
			const auto& pinType = pins[j];
			const auto circle = getNodeElementArea(*nodeType, basePos, node, j, curZoom, 1.0f).expand((pinPriority ? 12.0f : 4.0f) / curZoom);
			if (circle.contains(mousePos)) {
				foundPin = true;
				const float distance = (mousePos - circle.getCentre()).length();
				if (distance < bestDistance) {
					bestDistance = distance;
					bestResult = NodeUnderMouseInfo{ static_cast<ScriptNodeId>(i), pinType, static_cast<ScriptPinId>(j), curRect, circle.getCentre() };
				}
			}
		}
		
		// Check main body
		if (!foundPin && curRect.contains(mousePos)) {
			const float distance = (mousePos - curRect.getCenter()).length();
			if (distance < bestDistance) {
				bestDistance = distance;
				bestResult = NodeUnderMouseInfo{ static_cast<ScriptNodeId>(i), ScriptNodePinType{ScriptNodeElementType::Node}, static_cast<ScriptPinId>(-1), curRect, Vector2f() };
			}
		}
	}

	return bestResult;
}

Vector2f ScriptRenderer::getPinPosition(Vector2f basePos, const ScriptGraphNode& node, ScriptPinId idx, float zoom) const
{
	return getNodeElementArea(node.getNodeType(), basePos, node, idx, zoom, 1.0f).getCentre();
}

Vector<ScriptNodeId> ScriptRenderer::getNodesInRect(Vector2f basePos, float curZoom, Rect4f selBox) const
{
	if (!graph) {
		return {};
	}

	const float effectiveZoom = std::max(nativeZoom, curZoom);
	Vector<ScriptNodeId> result;

	for (size_t i = 0; i < graph->getNodes().size(); ++i) {
		const auto& node = graph->getNodes()[i];
		const auto pos = basePos + node.getPosition();

		const auto* nodeType = nodeTypeCollection.tryGetNodeType(node.getType());
		if (!nodeType) {
			continue;
		}
		const auto nodeSize = getNodeSize(*nodeType, effectiveZoom);
		const Rect4f area = Rect4f(-nodeSize / 2, nodeSize / 2) / effectiveZoom;
		const auto curRect = area + pos;

		if (curRect.overlaps(selBox)) {
			result.push_back(static_cast<ScriptNodeId>(i));
		}
	}

	return result;
}

void ScriptRenderer::setHighlight(std::optional<NodeUnderMouseInfo> node,OptionalLite<uint8_t> entity)
{
	highlightNode = std::move(node);
	highlightEntity = entity;
}

void ScriptRenderer::setSelection(Vector<ScriptNodeId> nodes)
{
	selectedNodes = std::move(nodes);
}

void ScriptRenderer::setCurrentPaths(Vector<ConnectionPath> path)
{
	currentPaths = std::move(path);
}
