#include "prefab_editor.h"

#include "src/editor_root_stage.h"
using namespace Halley;

PrefabEditor::PrefabEditor(UIFactory& factory, Resources& resources, AssetType type, Project& project, EditorRootStage& stage)
	: AssetEditor(factory, resources, project, type)
	, stage(stage)
{
	setupWindow();
}

void PrefabEditor::reload()
{
	
}

void PrefabEditor::onDoubleClick()
{
	stage.openPrefab(assetId, assetType);
}

void PrefabEditor::setupWindow()
{
	add(factory.makeUI("ui/halley/prefab_editor"), 1);

	setHandle(UIEventType::ButtonClicked, "open", [=](const UIEvent& event)
	{
		stage.openPrefab(assetId, assetType);
	});
}
