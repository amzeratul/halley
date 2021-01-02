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

void PrefabEditor::reload()
{
}

void PrefabEditor::onDoubleClick()
{
}

bool PrefabEditor::isModified()
{
	return window && window->isModified();
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
	open();
	return {};
}

void PrefabEditor::open()
{
	if (!project.isDLLLoaded()) {
		pendingLoad = true;
		return;
	}
	
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
