#include "prefab_editor.h"

#include "src/ui/project_window.h"
using namespace Halley;

PrefabEditor::PrefabEditor(UIFactory& factory, Resources& resources, AssetType type, Project& project, ProjectWindow& projectWindow)
	: AssetEditor(factory, resources, project, type)
	, projectWindow(projectWindow)
{
	setupWindow();
}

void PrefabEditor::reload()
{
}

void PrefabEditor::onDoubleClick()
{
	projectWindow.openPrefab(assetId, assetType);
}

void PrefabEditor::setupWindow()
{
	add(factory.makeUI("ui/halley/prefab_editor"), 1);

	setHandle(UIEventType::ButtonClicked, "open", [=](const UIEvent& event)
	{
		projectWindow.openPrefab(assetId, assetType);
	});
}
