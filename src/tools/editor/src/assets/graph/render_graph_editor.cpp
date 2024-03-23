#include "render_graph_editor.h"
#include "halley/tools/project/project.h"
using namespace Halley;

RenderGraphEditor::RenderGraphEditor(UIFactory& factory, Resources& gameResources, Project& project, AssetType type)
	: AssetEditor(factory, gameResources, project, type)
{
}

void RenderGraphEditor::onResourceLoaded()
{
	renderGraph = std::dynamic_pointer_cast<const RenderGraphDefinition>(resource);
}

std::shared_ptr<const Resource> RenderGraphEditor::loadResource(const Path& assetPath, const String& assetId, AssetType assetType)
{
	return std::make_shared<RenderGraphDefinition>(YAMLConvert::parseConfig(project.getAssetsSrcPath() / assetPath).getRoot());
}
