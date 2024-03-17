#include "asset_editor.h"

#include "halley/tools/project/project.h"

using namespace Halley;

AssetEditor::AssetEditor(UIFactory& factory, Resources& gameResources, Project& project, AssetType type)
	: UIWidget("assetEditor", {}, UISizer())
	, factory(factory)
	, project(project)
	, gameResources(gameResources)
	, assetType(type)
{
	setHandle(UIEventType::TabbedIn, [=] (const auto& event)
	{
		setTabbedIn(true);
	});

	setHandle(UIEventType::TabbedOut, [=] (const auto& event)
	{
		setTabbedIn(false);
	});
}

void AssetEditor::update(Time t, bool moved)
{
	tryLoading();
}

void AssetEditor::setResource(Path filePath, String assetId)
{
	this->assetPath = std::move(filePath);
	this->assetId = std::move(assetId);
	needsLoading = true;
}

void AssetEditor::onResourceLoaded()
{
}

void AssetEditor::refreshAssets()
{
	// If this is set to true every time, it causes scenes to re-load when anything is modified
	if (!resource) {
		needsLoading = true;
	}
	tryLoading();
}

void AssetEditor::onDoubleClick()
{
}

bool AssetEditor::isModified()
{
	return false;
}

void AssetEditor::save()
{
}

bool AssetEditor::canSave(bool forceInstantCheck) const
{
	return true;
}

void AssetEditor::onOpenAssetFinder(PaletteWindow& assetFinder)
{
}

bool AssetEditor::isReadyToLoad() const
{
	return true;
}

bool AssetEditor::needsDLLToLoad() const
{
	return true;
}

void AssetEditor::onTabbedIn()
{
}

void AssetEditor::setTabbedIn(bool value)
{
	tabbedIn = value;
	tryLoading();
	if (value) {
		onTabbedIn();
	}
}

void AssetEditor::tryLoading()
{
	if (tabbedIn && needsLoading && (!needsDLLToLoad() || project.isDLLLoaded()) && isReadyToLoad()) {
		load();
	}
}

void AssetEditor::load()
{
	try {
		needsLoading = false;
		resource = loadResource(assetPath, assetId, assetType);
		onResourceLoaded();
	} catch (const std::exception& e) {
		Logger::logException(e);
	}
}
