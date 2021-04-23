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
	nodeBg = Sprite().setImage(resources, "halley_ui/ui_float_solid_window.png");
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
	
	for (uint32_t i = 0; i < static_cast<uint32_t>(graph->getNodes().size()); ++i) {
		const auto& node = graph->getNodes()[i];
		
		NodeDrawMode mode = NodeDrawMode::Normal;
		if (highlightNode == i) {
			mode = NodeDrawMode::Highlight;
		} else if (state && !state->hasThreadAt(i)) {
			mode = NodeDrawMode::Dimmed;
		}
		
		drawNode(painter, basePos, node, effectiveZoom, mode);
	}
}

void ScriptRenderer::drawNodeOutputs(Painter& painter, Vector2f basePos, const ScriptGraphNode& node, const ScriptGraph& graph, float curZoom)
{
	const auto* nodeType = nodeTypeCollection.tryGetNodeType(node.getType());
	if (!nodeType) {
		return;
	}

	for (const auto& output: node.getOutputs()) {
		const size_t srcIdx = 0; // TODO		
		const Vector2f srcPos = getNodeElementPosition(*nodeType, NodeElementType::Output, basePos, node, srcIdx, curZoom);

		const size_t dstIdx = 0; // TODO
		const auto& dstNode = graph.getNodes().at(output.nodeId);
		const auto* dstNodeType = nodeTypeCollection.tryGetNodeType(dstNode.getType());
		if (!dstNodeType) {
			continue;
		}
		const Vector2f dstPos = getNodeElementPosition(*dstNodeType, NodeElementType::Input, basePos, dstNode, dstIdx, curZoom);

		const float dist = std::max(std::abs(dstPos.x - srcPos.x), 20.0f) / 2;
		const auto bezier = BezierCubic(srcPos, srcPos + Vector2f(dist, 0), dstPos - Vector2f(dist, 0), dstPos);
		
		painter.drawLine(bezier, 2.0f / curZoom, Colour4f(1, 1, 1));
	}

	for (size_t i = 0; i < node.getTargets().size(); ++i) {
		const auto& target = node.getTargets()[i];
		if (target.isValid()) {
			auto entity = world.getEntity(target);
			auto* transform = entity.tryGetComponent<Transform2DComponent>();
			if (transform) {
				const Vector2f srcPos = getNodeElementPosition(*nodeType, NodeElementType::Target, basePos, node, i, curZoom);
				const auto dstPos = transform->getGlobalPosition();

				const float dist = std::max(std::abs(dstPos.x - srcPos.x), 20.0f) / 2;
				const auto bezier = BezierCubic(srcPos, srcPos + Vector2f(0, dist), dstPos - Vector2f(0, dist), dstPos);
				
				painter.drawLine(bezier, 1.5f / curZoom, Colour4f(0.35f, 1.0f, 0.35f));
			}
		} else {
			Logger::logWarning("Invalid target on script graph node");
		}
	}
}

void ScriptRenderer::drawNode(Painter& painter, Vector2f basePos, const ScriptGraphNode& node, float curZoom, NodeDrawMode drawMode)
{
	const Vector2f border = Vector2f(18, 18);
	const Vector2f nodeSize = getNodeSize(curZoom);
	const auto pos = basePos + node.getPosition();

	const auto* nodeType = nodeTypeCollection.tryGetNodeType(node.getType());
	if (!nodeType) {
		return;
	}

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
		.setPivot(Vector2f(0.5f, 0.5f))
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

	const float radius = 4.0f / curZoom;
	const float width = 2.0f / curZoom;

	for (size_t i = 0; i < nodeType->getNumInputPins(); ++i) {
		painter.drawCircle(getNodeElementPosition(*nodeType, NodeElementType::Input, basePos, node, i, curZoom), radius, width, Colour4f(1, 1, 1));
	}

	for (size_t i = 0; i < nodeType->getNumOutputPins(); ++i) {
		painter.drawCircle(getNodeElementPosition(*nodeType, NodeElementType::Output, basePos, node, i, curZoom), radius, width, Colour4f(1, 1, 1));
	}
	
	for (size_t i = 0; i < nodeType->getNumTargetPins(); ++i) {
		painter.drawCircle(getNodeElementPosition(*nodeType, NodeElementType::Target, basePos, node, i, curZoom), radius, width, Colour4f(0.35f, 1, 0.35f));
	}
}

Vector2f ScriptRenderer::getNodeSize(float curZoom) const
{
	return Vector2f(60, 60);
}

Vector2f ScriptRenderer::getNodeElementPosition(const IScriptNodeType& nodeType, NodeElementType type, Vector2f basePos, const ScriptGraphNode& node, size_t elemIdx, float curZoom) const
{
	const Vector2f nodeSize = getNodeSize(curZoom);
	
	Vector2f offset;
	switch (type) {
	case NodeElementType::Input:
		offset = Vector2f(-nodeSize.x * 0.5f, 0.0f);
		break;
	case NodeElementType::Output:
		offset = Vector2f(nodeSize.x * 0.5f, 0.0f);
		break;
	case NodeElementType::Target:
		offset = Vector2f(0.0f, nodeSize.y * 0.5f);
		break;
	}

	const Vector2f pos = basePos + node.getPosition();
	return pos + offset / curZoom;
}

Colour4f ScriptRenderer::getNodeColour(const IScriptNodeType& nodeType) const
{
	switch (nodeType.getClassification()) {
	case ScriptNodeClassification::Terminator:
		return Colour4f(0.97f, 0.35f, 0.35f);
	case ScriptNodeClassification::Action:
		return Colour4f(0.07f, 0.84f, 0.09f);
	case ScriptNodeClassification::Condition:
		return Colour4f(0.91f, 0.71f, 0.0f);
	case ScriptNodeClassification::FlowControl:
		return Colour4f(0.35f, 0.35f, 0.97f);
	}
	return Colour4f(0.2f, 0.2f, 0.2f);
}

const Sprite& ScriptRenderer::getIcon(const IScriptNodeType& nodeType)
{
	const auto iter = icons.find(nodeType.getName());
	if (iter != icons.end()) {
		return iter->second;
	}
	icons[nodeType.getName()] = Sprite().setImage(resources, nodeType.getIconName()).setPivot(Vector2f(0.5f, 0.5f));
	return icons[nodeType.getName()];
}

std::optional<uint32_t> ScriptRenderer::getNodeIdxUnderMouse(Vector2f basePos, float curZoom, std::optional<Vector2f> mousePos) const
{
	if (!graph || !mousePos) {
		return {};
	}

	const float effectiveZoom = std::max(nativeZoom, curZoom);
	const auto nodeSize = getNodeSize(effectiveZoom);
	const Rect4f area = Rect4f(-nodeSize / 2, nodeSize / 2) / effectiveZoom;

	for (size_t i = 0; i < graph->getNodes().size(); ++i) {
		const auto& node = graph->getNodes()[i];
		const auto pos = basePos + node.getPosition();
		if ((area + pos).contains(mousePos.value())) {
			return static_cast<uint32_t>(i);
		}
	}

	return {};
}

void ScriptRenderer::setHighlight(std::optional<uint32_t> node)
{
	highlightNode = node;
}
