#include "asset_editor.h"

using namespace Halley;

Halley::AssetEditor::AssetEditor(UIFactory& factory, Resources& gameResources, Project& project, AssetType type)
	: UIWidget("assetEditor", {}, UISizer())
	, factory(factory)
	, project(project)
	, gameResources(gameResources)
	, assetType(type)
{
}

void AssetEditor::setResource(const String& id)
{
	assetId = id;
	try {
		resource = loadResource(id);
		reload();
	} catch (const std::exception& e) {
		Logger::logException(e);
	}
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
	if (!resource) {
		resource = loadResource(assetId);
		reload();
	}
}

void AssetEditor::onDoubleClick()
{
}

bool AssetEditor::isModified()
{
	return false;
}
