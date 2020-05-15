#include "toolbar.h"

#include "project_window.h"
#include "halley/tools/project/build_project_task.h"
#include "halley/tools/project/project.h"
#include "halley/tools/project/project_properties.h"
#include "halley/ui/ui_sizer.h"
#include "halley/ui/widgets/ui_label.h"
#include "halley/text/string_converter.h"

using namespace Halley;

Toolbar::Toolbar(UIFactory& factory, ProjectWindow& projectWindow, Project& project)
	: UIWidget("toolbar", {}, UISizer())
	, factory(factory)
	, projectWindow(projectWindow)
	, project(project)
{
	makeUI();
}

const std::shared_ptr<UIList>& Toolbar::getList() const
{
	return list;
}

void Toolbar::makeUI()
{
	add(factory.makeUI("ui/halley/toolbar"), 1);

	getWidgetAs<UILabel>("gameName")->setText(LocalisedString::fromUserString(project.getProperties().getName()));
	list = getWidgetAs<UIList>("toolbarList");

	setHandle(UIEventType::ListSelectionChanged, "toolbarList", [=] (const UIEvent& event)
	{
		String toolName;
		const auto& tabId = event.getStringData();
		const auto& toolNameWidget = getWidgetAs<UILabel>("toolName");

		auto tabNames = EnumNames<EditorTabs>()();
		if (std::find_if(tabNames.begin(), tabNames.end(), [&](const char* v) { return tabId == v; }) == tabNames.end()) {
			const auto name = projectWindow.setCustomPage(tabId);
			toolNameWidget->setText(LocalisedString::fromHardcodedString(name));
			return;
		}
		
		const auto tab = fromString<EditorTabs>(tabId);
		
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
		toolNameWidget->setText(LocalisedString::fromHardcodedString(toolName));
		projectWindow.setPage(tab);
	});
	
	setHandle(UIEventType::ButtonClicked, "exitProject", [=] (const UIEvent& event)
	{
		projectWindow.destroy();
	});

	setHandle(UIEventType::ButtonClicked, "runProject", [=] (const UIEvent& event)
	{
		OS::get().runCommandAsync("\"" + project.getExecutablePath().getNativeString() + "\" --devcon=127.0.0.1");
	});

	setHandle(UIEventType::ButtonClicked, "buildProject", [=] (const UIEvent& event)
	{
		projectWindow.getTasks().addTask(EditorTaskAnchor(std::make_unique<BuildProjectTask>(project)));
	});
}
