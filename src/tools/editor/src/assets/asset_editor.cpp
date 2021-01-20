#include "asset_editor.h"

using namespace Halley;

Halley::AssetEditor::AssetEditor(UIFactory& factory, Resources& resources, Project& project, AssetType type)
	: UIWidget("assetEditor", {}, UISizer())
	, factory(factory)
	, project(project)
	, gameResources(resources)
	, assetType(type)
{
}

void AssetEditor::setResource(const String& id)
{
	assetId = id;
	resource = loadResource(id);
	reload();
}

void AssetEditor::clearResource()
{
	assetId = "";
	resource.reset();
	reload();
}

void AssetEditor::reload()
{
}

void AssetEditor::refreshAssets()
{
}

void AssetEditor::onDoubleClick()
{
}

bool AssetEditor::isModified()
{
	return false;
}
