#include "scripting/script_renderer.h"


#include "halley/core/graphics/painter.h"
#include "halley/maths/bezier.h"
#include "scripting/script_graph.h"
using namespace Halley;

ScriptRenderer::ScriptRenderer(Resources& resources)
	: resources(resources)
{
	nodeBg = Sprite().setImage(resources, "halley_ui/ui_float_solid_window.png");
}

void ScriptRenderer::setGraph(const ScriptGraph& graph)
{
	this->graph = &graph;
}

void ScriptRenderer::draw(Painter& painter, Vector2f basePos, float curZoom)
{
	if (!graph) {
		return;
	}

	for (const auto& node: graph->getNodes()) {
		drawNodeOutputs(painter, basePos, node, *graph, curZoom);
	}
	
	for (const auto& node: graph->getNodes()) {
		drawNode(painter, basePos, node, curZoom);
	}
}

void ScriptRenderer::drawNodeOutputs(Painter& painter, Vector2f basePos, const ScriptGraphNode& node, const ScriptGraph& graph, float curZoom)
{
	const Vector2f pos = basePos + node.getPosition();
	const Vector2f nodeSize = getNodeSize(curZoom);
	const Vector2f outputPinPos = pos + Vector2f(nodeSize.x * 0.5f, 0.0f) / curZoom;

	for (const auto& output: node.getOutputs()) {
		const auto& dstNode = graph.getNodes().at(output.nodeId);
		const Vector2f inputPinPos = basePos + dstNode.getPosition() - Vector2f(nodeSize.x * 0.5f, 0.0f) / curZoom;

		const float dist = std::max(std::abs(inputPinPos.x - outputPinPos.x), 50.0f) / 2;
		const auto bezier = BezierCubic(outputPinPos, outputPinPos + Vector2f(dist, 0), inputPinPos - Vector2f(dist, 0), inputPinPos);
		
		painter.drawLine(bezier, 2.0f / curZoom, Colour4f(1, 1, 1));
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
}

Vector2f ScriptRenderer::getNodeSize(float curZoom) const
{
	return Vector2f(60, 60);
}
