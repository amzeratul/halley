#include "render_graph_editor.h"
using namespace Halley;

RenderGraphEditor::RenderGraphEditor(UIFactory& factory, Resources& gameResources, Project& project, AssetType type)
	: GraphEditor(factory, gameResources, project, type)
{
}

void RenderGraphEditor::reload()
{
	const auto renderGraph = std::dynamic_pointer_cast<const RenderGraphDefinition>(resource);

	int i = 0;
	for (const auto& node: renderGraph->getNodes()) {
		addNode(Vector2f(400, 50) * i + Vector2f(-150, 200));
		++i;
	}
}

std::shared_ptr<const Resource> RenderGraphEditor::loadResource(const String& assetId)
{
	return gameResources.get<RenderGraphDefinition>(assetId);
}
