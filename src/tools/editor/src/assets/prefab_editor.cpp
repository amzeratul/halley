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
	return window && window->isModified();
}

void PrefabEditor::save()
{
	if (window) {
		window->saveScene();
	}
}

bool PrefabEditor::canSave(bool forceInstantCheck) const
{
	return window && window->canSave(forceInstantCheck);
}

void PrefabEditor::update(Time t, bool moved)
{
	if (pendingLoad && project.isDLLLoaded()) {
		pendingLoad = false;
		open();
	}
}

std::shared_ptr<const Resource> PrefabEditor::loadResource(const String& assetId)
{
	if (project.isDLLLoaded()) {
		open();
	} else {
		pendingLoad = true;
	}
	
	return {};
}

void PrefabEditor::onTabbedIn()
{
	if (window) {
		window->onTabbedIn();
	}
}

void PrefabEditor::open()
{
	Expects (project.isDLLLoaded());
	
	if (!window) {
		window = std::make_shared<SceneEditorWindow>(factory, project, projectWindow.getAPI(), projectWindow);
		add(window, 1);
	}
	if (assetType == AssetType::Scene) {
		window->loadScene(assetId);
	} else if (assetType == AssetType::Prefab) {
		window->loadPrefab(assetId);
	}
}
