#include "render_graph_editor.h"
using namespace Halley;

RenderGraphEditor::RenderGraphEditor(UIFactory& factory, Resources& resources, Project& project, AssetType type)
	: GraphEditor(factory, resources, project, type)
{
}

std::shared_ptr<const Resource> RenderGraphEditor::loadResource(const String& assetId)
{
	return gameResources.get<RenderGraphDefinition>(assetId);
}
