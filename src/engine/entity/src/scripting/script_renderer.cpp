#include "scripting/script_renderer.h"

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
	if (graph) {
		for (const auto& node: graph->getNodes()) {
			drawNode(painter, basePos, node, curZoom);
		}
	}
}

void ScriptRenderer::drawNode(Painter& painter, Vector2f basePos, const ScriptGraphNode& node, float curZoom)
{
	const auto pos = basePos + node.getPosition();
	nodeBg.clone()
		.setPivot(Vector2f(0.5f, 0.5f))
		.setPosition(pos)
		.setScale(1.0f / curZoom)
		.draw(painter);
}
