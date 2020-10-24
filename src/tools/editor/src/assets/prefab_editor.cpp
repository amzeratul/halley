#include "prefab_editor.h"

#include "src/ui/project_window.h"
using namespace Halley;

PrefabEditor::PrefabEditor(UIFactory& factory, Resources& resources, AssetType type, Project& project, ProjectWindow& projectWindow)
	: AssetEditor(factory, resources, project, type)
	, project(project)
	, projectWindow(projectWindow)
{
	setupWindow();
}

void PrefabEditor::reload()
{
}

void PrefabEditor::onDoubleClick()
{
	open();
}

void PrefabEditor::update(Time t, bool moved)
{
	updateButton();
}

std::shared_ptr<const Resource> PrefabEditor::loadResource(const String& assetId)
{
	return {};
}

void PrefabEditor::setupWindow()
{
	add(factory.makeUI("ui/halley/prefab_editor"), 1);

	setHandle(UIEventType::ButtonClicked, "open", [=](const UIEvent& event)
	{
		open();
	});

	updateButton();
}

void PrefabEditor::open()
{
	if (project.isDLLLoaded()) {
		projectWindow.openPrefab(assetId, assetType);
	}
}

void PrefabEditor::updateButton()
{
	const bool enabled = project.isDLLLoaded();
	getWidgetAs<UIButton>("open")->setActive(enabled);
	getWidgetAs<UIButton>("openDisabled")->setActive(!enabled);
}
