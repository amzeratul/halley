#include "ecs_window.h"


#include "halley/tools/ecs/ecs_data.h"
#include "halley/tools/project/project.h"
using namespace Halley;

ECSWindow::ECSWindow(UIFactory& factory, Project& project)
	: UIWidget("ecs_window", Vector2f(), UISizer())
	, project(project)
{
	factory.loadUI(*this, "ui/halley/ecs");
}

void ECSWindow::onMakeUI()
{
	auto systemList = getWidgetAs<UIList>("systemList");
	auto componentList = getWidgetAs<UIList>("componentList");
	const auto& ecsData = project.getECSData();
	
	for (const auto& system: ecsData.getSystems()) {
		systemList->addTextItem(system.first, LocalisedString::fromUserString(system.first));
	}
	
	for (const auto& system: ecsData.getComponents()) {
		componentList->addTextItem(system.first, LocalisedString::fromUserString(system.first));
	}

	setHandle(UIEventType::ListSelectionChanged, "tabs", [=] (const UIEvent& event)
	{
		getWidgetAs<UIPagedPane>("content")->setPage(event.getIntData());
	});
}
