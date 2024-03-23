#include "prefab_editor.h"
#include "src/scene/scene_editor_window.h"
#include "src/ui/project_window.h"
using namespace Halley;

PrefabEditor::PrefabEditor(UIFactory& factory, Resources& resources, AssetType type, Project& project, ProjectWindow& projectWindow)
	: AssetEditor(factory, resources, project, type)
	, project(project)
	, projectWindow(projectWindow)
{
}

void PrefabEditor::refreshAssets()
{
	AssetEditor::refreshAssets();
	if (window) {
		window->refreshAssets();
	}
}

void PrefabEditor::onDoubleClick()
{
}

bool PrefabEditor::isModified()
{
	return (window && window->isModified()) || (!drillDown.empty() && drillDown.back()->isModified());
}

void PrefabEditor::save()
{
	if (window) {
		if (!drillDown.empty()) {
			drillDown.back()->drillDownSave();
		}
		window->saveScene();
	}
}

bool PrefabEditor::canSave(bool forceInstantCheck) const
{
	return window && window->canSave(forceInstantCheck);
}

void PrefabEditor::onOpenAssetFinder(PaletteWindow& assetFinder)
{
	if (window) {
		window->onOpenAssetFinder(assetFinder);
	}
}

void PrefabEditor::drillDownEditor(std::shared_ptr<IDrillDownAssetWindow> editor)
{
	for (auto& otherDrill: drillDown) {
		otherDrill->asWidget()->setActive(false);
	}
	add(editor->asWidget(), 1);
	drillDown.push_back(std::move(editor));
	window->setActive(false);
	layout();
}

bool PrefabEditor::isReadyToLoad() const
{
	return project.areAssetsLoaded() && elapsedTime >= minLoadTime;
}

void PrefabEditor::update(Time t, bool moved)
{
	AssetEditor::update(t, moved);

	if (!drillDown.empty() && !drillDown.back()->asWidget()->isAlive()) {
		drillDown.pop_back();
	}
	if (drillDown.empty()) {
		if (window) {
			window->setActive(true);
		}
	} else {
		drillDown.back()->asWidget()->setActive(true);
	}
	elapsedTime += t;
}

void PrefabEditor::onTabbedIn()
{
	if (window) {
		window->onTabbedIn();
	}
}

std::shared_ptr<const Resource> PrefabEditor::loadResource(const Path& assetPath, const String& assetId, AssetType assetType)
{
	if (!window) {
		window = std::make_shared<SceneEditorWindow>(factory, project, projectWindow.getAPI(), projectWindow, *this, assetType);
		add(window, 1);
	}

	const auto prefab = assetType == AssetType::Prefab ? std::make_shared<Prefab>() : std::make_shared<Scene>();
	
	const auto assetData = Path::readFile(project.getAssetsSrcPath() / assetPath);
	if (!assetData.empty()) {
		prefab->parseYAML(gsl::as_bytes(gsl::span<const Byte>(assetData)));
	} else {
		prefab->makeDefault();
	}
	prefab->setAssetId(assetId);

	window->setScene(prefab, assetPath);

	return prefab;
}
