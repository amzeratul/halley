#include "scripting/script_renderer.h"
#include "world.h"
#include "halley/core/graphics/painter.h"
#include "halley/maths/bezier.h"
#include "halley/support/logger.h"
#include "scripting/script_graph.h"
using namespace Halley;

#ifndef DONT_INCLUDE_HALLEY_HPP
#define DONT_INCLUDE_HALLEY_HPP
#endif
#include "components/transform_2d_component.h"

ScriptRenderer::ScriptRenderer(Resources& resources, World& world, const ScriptNodeTypeCollection& nodeTypeCollection)
	: resources(resources)
	, world(world)
	, nodeTypeCollection(nodeTypeCollection)
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

	const float effectiveZoom = std::max(1.0f, curZoom);

	for (const auto& node: graph->getNodes()) {
		drawNodeOutputs(painter, basePos, node, *graph, effectiveZoom);
	}
	
	for (const auto& node: graph->getNodes()) {
		drawNode(painter, basePos, node, effectiveZoom);
	}
}

void ScriptRenderer::drawNodeOutputs(Painter& painter, Vector2f basePos, const ScriptGraphNode& node, const ScriptGraph& graph, float curZoom)
{
	const Vector2f pos = basePos + node.getPosition();

	for (const auto& output: node.getOutputs()) {
		const Vector2f srcPos = getNodeElementPosition(NodeElementType::Output, basePos, node, 0, curZoom);
		
		const auto& dstNode = graph.getNodes().at(output.nodeId);
		const Vector2f dstPos = getNodeElementPosition(NodeElementType::Input, basePos, dstNode, 0, curZoom);

		const float dist = std::max(std::abs(dstPos.x - srcPos.x), 20.0f) / 2;
		const auto bezier = BezierCubic(srcPos, srcPos + Vector2f(dist, 0), dstPos - Vector2f(dist, 0), dstPos);
		
		painter.drawLine(bezier, 2.0f / curZoom, Colour4f(1, 1, 1));
	}

	for (const auto& target: node.getTargets()) {
		if (target.isValid()) {
			auto entity = world.getEntity(target);
			auto* transform = entity.tryGetComponent<Transform2DComponent>();
			if (transform) {
				const Vector2f srcPos = getNodeElementPosition(NodeElementType::Target, basePos, node, 0, curZoom);
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

void ScriptRenderer::drawNode(Painter& painter, Vector2f basePos, const ScriptGraphNode& node, float curZoom)
{
	const Vector2f border = Vector2f(18, 18);
	const Vector2f nodeSize = getNodeSize(curZoom);
	const auto pos = basePos + node.getPosition();

	// Node body
	nodeBg.clone()
		.setColour(Colour4f(0.35f, 0.35f, 0.97f))
		.setPivot(Vector2f(0.5f, 0.5f))
		.setPosition(pos)
		.scaleTo(nodeSize + border)
		.setSize(nodeBg.getSize() / curZoom)
		.setSliceScale(1.0f / curZoom)
		//.setTexRect(nodeBg.getTexRect() / curZoom)
		.draw(painter);

	const float radius = 4.0f / curZoom;
	const float width = 2.0f / curZoom;
	painter.drawCircle(getNodeElementPosition(NodeElementType::Input, basePos, node, 0, curZoom), radius, width, Colour4f(1, 1, 1));
	painter.drawCircle(getNodeElementPosition(NodeElementType::Output, basePos, node, 0, curZoom), radius, width, Colour4f(1, 1, 1));
	if (!node.getTargets().empty()) {
		painter.drawCircle(getNodeElementPosition(NodeElementType::Target, basePos, node, 0, curZoom), radius, width, Colour4f(0.35f, 1, 0.35f));
	}
}

Vector2f ScriptRenderer::getNodeSize(float curZoom) const
{
	return Vector2f(60, 60);
}

Vector2f ScriptRenderer::getNodeElementPosition(NodeElementType type, Vector2f basePos, const ScriptGraphNode& node, size_t elemIdx, float curZoom)
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
