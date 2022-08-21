#include "halley/core/graph/base_graph_renderer.h"
using namespace Halley;


void BaseGraphRenderer::setHighlight(std::optional<NodeUnderMouseInfo> node,OptionalLite<uint8_t> entity)
{
	highlightNode = std::move(node);
	highlightEntity = entity;
}

void BaseGraphRenderer::setSelection(Vector<GraphNodeId> nodes)
{
	selectedNodes = std::move(nodes);
}

void BaseGraphRenderer::setCurrentPaths(Vector<ConnectionPath> path)
{
	currentPaths = std::move(path);
}
