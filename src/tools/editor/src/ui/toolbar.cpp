#include "toolbar.h"


#include "halley/tools/project/build_project_task.h"
#include "halley/tools/project/project.h"
#include "halley/tools/project/project_properties.h"
#include "halley/ui/ui_sizer.h"
#include "halley/ui/widgets/ui_label.h"
#include "src/editor_root_stage.h"

using namespace Halley;

Toolbar::Toolbar(UIFactory& factory, EditorRootStage& editorStage, Project& project)
	: UIWidget("toolbar", {}, UISizer())
	, factory(factory)
	, editorStage(editorStage)
	, project(project)
{
	makeUI();
}

void Toolbar::makeUI()
{
	add(factory.makeUI("ui/halley/toolbar"), 1);

	getWidgetAs<UILabel>("gameName")->setText(LocalisedString::fromUserString(project.getProperties().getName()));

	setHandle(UIEventType::ListSelectionChanged, "toolbarList", [=] (const UIEvent& event)
	{
		String toolName;
		auto tab = EditorTabs(event.getIntData());
		
		switch (tab) {
		case EditorTabs::Assets:
			toolName = "Assets";
			break;
		case EditorTabs::Scene:
			toolName = "Scene";
			break;
		case EditorTabs::ECS:
			toolName = "ECS";
			break;
		case EditorTabs::Remotes:
			toolName = "Remotes";
			break;
		case EditorTabs::Properties:
			toolName = "Properties";
			break;
		case EditorTabs::Settings:
			toolName = "Settings";
			break;
		}
		getWidgetAs<UILabel>("toolName")->setText(LocalisedString::fromHardcodedString(toolName));
		editorStage.setPage(tab);
	});
	
	setHandle(UIEventType::ButtonClicked, "exitProject", [=] (const UIEvent& event)
	{
		editorStage.createLoadProjectUI();
	});

	setHandle(UIEventType::ButtonClicked, "runProject", [=] (const UIEvent& event)
	{
		OS::get().runCommandAsync("\"" + project.getExecutablePath().getNativeString() + "\" --devcon=127.0.0.1");
	});

	setHandle(UIEventType::ButtonClicked, "buildProject", [=] (const UIEvent& event)
	{
		editorStage.getTasks().addTask(EditorTaskAnchor(std::make_unique<BuildProjectTask>(project)));
	});
}
