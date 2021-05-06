#include "scripting/script_renderer.h"
#include "world.h"
#include "halley/core/graphics/painter.h"
#include "halley/maths/bezier.h"
#include "halley/support/logger.h"
#include "scripting/script_graph.h"
#include "scripting/script_node_type.h"
using namespace Halley;

#ifndef DONT_INCLUDE_HALLEY_HPP
#define DONT_INCLUDE_HALLEY_HPP
#endif
#include "components/transform_2d_component.h"

ScriptRenderer::ScriptRenderer(Resources& resources, World& world, const ScriptNodeTypeCollection& nodeTypeCollection, float nativeZoom)
	: resources(resources)
	, world(world)
	, nodeTypeCollection(nodeTypeCollection)
	, nativeZoom(nativeZoom)
{
	nodeBg = Sprite().setImage(resources, "halley_ui/ui_float_solid_window.png").setPivot(Vector2f(0.5f, 0.5f));
	pinSprite = Sprite().setImage(resources, "halley_ui/ui_render_graph_node_pin.png").setPivot(Vector2f(0.5f, 0.5f));
}

void ScriptRenderer::setGraph(const ScriptGraph* graph)
{
	this->graph = graph;
}

void ScriptRenderer::setState(const ScriptState* scriptState)
{
	this->state = scriptState;
}

void ScriptRenderer::draw(Painter& painter, Vector2f basePos, float curZoom)
{
	if (!graph) {
		return;
	}

	const float effectiveZoom = std::max(nativeZoom, curZoom);

	for (const auto& node: graph->getNodes()) {
		drawNodeOutputs(painter, basePos, node, *graph, effectiveZoom);
	}

	if (currentPath) {
		drawConnection(painter, currentPath.value(), curZoom);
	}
	
	for (uint32_t i = 0; i < static_cast<uint32_t>(graph->getNodes().size()); ++i) {
		const auto& node = graph->getNodes()[i];

		const bool highlightThis = highlightNode && highlightNode->nodeId == i;
		
		NodeDrawMode mode = NodeDrawMode::Normal;
		if (highlightThis && highlightNode->element.type == ScriptNodeElementType::Node) {
			mode = NodeDrawMode::Highlight;
		} else if (state && !state->hasThreadAt(i)) {
			mode = NodeDrawMode::Dimmed;
		}
		
		drawNode(painter, basePos, node, effectiveZoom, mode, highlightThis ? highlightNode->element : std::optional<ScriptNodePinType>(), highlightThis ? highlightNode->elementId : 0);
	}
}

void ScriptRenderer::drawNodeOutputs(Painter& painter, Vector2f basePos, const ScriptGraphNode& node, const ScriptGraph& graph, float curZoom)
{
	const auto* nodeType = nodeTypeCollection.tryGetNodeType(node.getType());
	if (!nodeType) {
		return;
	}

	for (size_t i = 0; i < node.getPins().size(); ++i) {
		const auto& srcPinType = nodeType->getPin(i);
		const auto& pin = node.getPins()[i];
		
		std::optional<Vector2f> dstPos;
		ScriptNodePinType dstPinType;
		
		if (srcPinType.type == ScriptNodeElementType::TargetPin) {
			const auto target = node.getPin(i).entity;
			if (target.isValid()) {
				auto entity = world.getEntity(target);
				auto* transform = entity.tryGetComponent<Transform2DComponent>();
				if (transform) {
					dstPos = transform->getGlobalPosition();
				}
			}
		} else if (pin.dstNode && srcPinType.direction == ScriptNodePinDirection::Output) {
			const size_t dstIdx = pin.dstPin;
			const auto& dstNode = graph.getNodes().at(pin.dstNode.value());
			const auto* dstNodeType = nodeTypeCollection.tryGetNodeType(dstNode.getType());
			if (!dstNodeType) {
				continue;
			}
			dstPos = getNodeElementArea(*dstNodeType, basePos, dstNode, dstIdx, curZoom).getCentre();
			dstPinType = dstNodeType->getPin(dstIdx);
		}

		if (dstPos) {
			const Vector2f srcPos = getNodeElementArea(*nodeType, basePos, node, i, curZoom).getCentre();
			drawConnection(painter, ConnectionPath{ srcPos, dstPos.value(), srcPinType, dstPinType }, curZoom);
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

void ScriptRenderer::drawConnection(Painter& painter, const ConnectionPath& path, float curZoom) const
{
	painter.drawLine(makeBezier(path), 1.5f / curZoom, getPinColour(path.fromType));
}

void ScriptRenderer::drawNode(Painter& painter, Vector2f basePos, const ScriptGraphNode& node, float curZoom, NodeDrawMode drawMode, std::optional<ScriptNodePinType> highlightElement, uint8_t highlightElementId)
{
	const Vector2f border = Vector2f(18, 18);
	const Vector2f nodeSize = getNodeSize(curZoom);
	const auto pos = basePos + node.getPosition();

	const auto* nodeType = nodeTypeCollection.tryGetNodeType(node.getType());
	if (!nodeType) {
		return;
	}

	{
		const auto baseCol = getNodeColour(*nodeType);
		Colour4f col = baseCol;
		switch (drawMode) {
		case NodeDrawMode::Highlight:
			col = col.inverseMultiplyLuma(0.5f);
			break;
		case NodeDrawMode::Dimmed:
			col = col.multiplyLuma(0.5f);
			break;
		}
		
		// Node body
		nodeBg.clone()
			.setColour(col)
			.setPosition(pos)
			.scaleTo(nodeSize + border)
			.setSize(nodeBg.getSize() / curZoom)
			.setSliceScale(1.0f / curZoom)
			//.setTexRect(nodeBg.getTexRect() / curZoom)
			.draw(painter);

		getIcon(*nodeType).clone()
			.setPosition(pos)
			.setScale(1.0f / curZoom)
			.draw(painter);
	}

	// Draw pins
	const auto& pins = nodeType->getPinConfiguration();
	for (size_t i = 0; i < pins.size(); ++i) {
		const auto& pinType = pins[i];
		const auto circle = getNodeElementArea(*nodeType, basePos, node, i, curZoom);
		const auto baseCol = getPinColour(pinType);
		const auto col = highlightElement == pinType && highlightElementId == i ? baseCol.inverseMultiplyLuma(0.3f) : baseCol;
		pinSprite.clone()
			.setPosition(circle.getCentre())
			.setColour(col)
			.setScale(1.0f / curZoom)
			.draw(painter);
	}
}

Vector2f ScriptRenderer::getNodeSize(float curZoom) const
{
	return Vector2f(60, 60);
}

Circle ScriptRenderer::getNodeElementArea(const IScriptNodeType& nodeType, Vector2f basePos, const ScriptGraphNode& node, size_t pinN, float curZoom) const
{
	const Vector2f nodeSize = getNodeSize(curZoom);
	const auto getOffset = [&] (size_t idx, size_t n)
	{
		const float spacing = 10.0f;
		return (static_cast<float>(idx) - (n - 1) * 0.5f) * spacing;
	};

	const auto& pin = nodeType.getPin(pinN);
	const auto pinSide = pin.getSide();
	
	size_t pinsOnSide = 0;
	size_t idxOnSide = 0;
	const auto& pins = nodeType.getPinConfiguration();
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
	}
	
	const Vector2f pos = basePos + node.getPosition();
	const Vector2f centre = pos + offset / curZoom;
	const float radius = 4.0f / curZoom;
	
	return Circle(centre, radius);
}

Colour4f ScriptRenderer::getNodeColour(const IScriptNodeType& nodeType) const
{
	switch (nodeType.getClassification()) {
	case ScriptNodeClassification::Terminator:
		return Colour4f(0.97f, 0.35f, 0.35f);
	case ScriptNodeClassification::Action:
		return Colour4f(0.07f, 0.84f, 0.09f);
	case ScriptNodeClassification::Variable:
		return Colour4f(0.91f, 0.71f, 0.0f);
	case ScriptNodeClassification::FlowControl:
		return Colour4f(0.35f, 0.35f, 0.97f);
	}
	return Colour4f(0.2f, 0.2f, 0.2f);
}

Colour4f ScriptRenderer::getPinColour(ScriptNodePinType pinType) const
{
	switch (pinType.type) {
	case ScriptNodeElementType::FlowPin:
		return Colour4f(0.8f, 0.8f, 0.8f);
	case ScriptNodeElementType::DataPin:
		return Colour4f(0.91f, 0.55f, 0.2f);
	case ScriptNodeElementType::TargetPin:
		return Colour4f(0.35f, 1, 0.35f);
	}

	return Colour4f();
}

const Sprite& ScriptRenderer::getIcon(const IScriptNodeType& nodeType)
{
	const auto iter = icons.find(nodeType.getId());
	if (iter != icons.end()) {
		return iter->second;
	}
	icons[nodeType.getId()] = Sprite().setImage(resources, nodeType.getIconName()).setPivot(Vector2f(0.5f, 0.5f));
	return icons[nodeType.getId()];
}

std::optional<ScriptRenderer::NodeUnderMouseInfo> ScriptRenderer::getNodeUnderMouse(Vector2f basePos, float curZoom, std::optional<Vector2f> mousePos, bool pinPriority) const
{
	if (!graph || !mousePos) {
		return {};
	}

	const float effectiveZoom = std::max(nativeZoom, curZoom);
	const auto nodeSize = getNodeSize(effectiveZoom);
	const Rect4f area = Rect4f(-nodeSize / 2, nodeSize / 2) / effectiveZoom;

	float bestDistance = std::numeric_limits<float>::max();
	std::optional<NodeUnderMouseInfo> bestResult;
	
	for (size_t i = 0; i < graph->getNodes().size(); ++i) {
		const auto& node = graph->getNodes()[i];
		const auto pos = basePos + node.getPosition();

		const auto nodeBounds = Circle(pos, area.getSize().length() / 2);
		if (!nodeBounds.contains(mousePos.value())) {
			continue;
		}
		
		const auto* nodeType = nodeTypeCollection.tryGetNodeType(node.getType());
		if (!nodeType) {
			continue;
		}
		const auto curRect = area + pos;
		
		// Check each pin handle
		bool foundPin = false;
		const auto& pins = nodeType->getPinConfiguration();
		for	(size_t j = 0; j < pins.size(); ++j) {
			const auto& pinType = pins[j];
			const auto circle = getNodeElementArea(*nodeType, basePos, node, j, curZoom).expand((pinPriority ? 12.0f : 4.0f) / curZoom);
			if (circle.contains(mousePos.value())) {
				foundPin = true;
				const float distance = (mousePos.value() - circle.getCentre()).length();
				if (distance < bestDistance) {
					bestDistance = distance;
					bestResult = NodeUnderMouseInfo{ static_cast<uint32_t>(i), pinType, static_cast<uint8_t>(j), curRect, circle.getCentre() };
				}
			}
		}
		
		// Check main body
		if (!foundPin && curRect.contains(mousePos.value())) {
			const float distance = (mousePos.value() - curRect.getCenter()).length();
			if (distance < bestDistance) {
				bestDistance = distance;
				bestResult = NodeUnderMouseInfo{ static_cast<uint32_t>(i), ScriptNodePinType{ScriptNodeElementType::Node}, 0, curRect, Vector2f() };
			}
		}
	}

	return bestResult;
}

void ScriptRenderer::setHighlight(std::optional<NodeUnderMouseInfo> node)
{
	highlightNode = node;
}

void ScriptRenderer::setCurrentPath(std::optional<ConnectionPath> path)
{
	currentPath = path;
}
