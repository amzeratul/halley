#include "ecs_window.h"


#include "halley/tools/ecs/ecs_data.h"
#include "halley/tools/project/project.h"
using namespace Halley;

ECSWindow::ECSWindow(UIFactory& factory, Project& project)
	: UIWidget("ecs_window", Vector2f(), UISizer())
	, factory(factory)
	, project(project)
{
	factory.loadUI(*this, "ui/halley/ecs");
}

void ECSWindow::onMakeUI()
{
	auto systemList = getWidgetAs<UIList>("systemList");
	auto componentList = getWidgetAs<UIList>("componentList");
	const auto& ecsData = project.getECSData();

	setHandle(UIEventType::ListSelectionChanged, "systemList", [=] (const UIEvent& event)
	{
		populateSystem(event.getStringData());
	});
	
	setHandle(UIEventType::ListSelectionChanged, "componentList", [=] (const UIEvent& event)
	{
		populateComponent(event.getStringData());
	});

	std::vector<String> systemNames;
	for (const auto& system: ecsData.getSystems()) {
		systemNames.push_back(system.first);
	}
	std::sort(systemNames.begin(), systemNames.end());
	for (const auto& system: systemNames) {
		systemList->addTextItem(system, LocalisedString::fromUserString(system));
	}

	std::vector<String> componentNames;
	for (const auto& component: ecsData.getComponents()) {
		componentNames.push_back(component.first);
	}
	std::sort(componentNames.begin(), componentNames.end());
	for (const auto& component: componentNames) {
		componentList->addTextItem(component, LocalisedString::fromUserString(component));
	}

	setHandle(UIEventType::ListSelectionChanged, "tabs", [=] (const UIEvent& event)
	{
		getWidgetAs<UIPagedPane>("content")->setPage(event.getIntData());
	});
}

void ECSWindow::populateSystem(const String& name)
{
	const auto& ecsData = project.getECSData();
	const auto& system = ecsData.getSystems().at(name);

	std::set<String> writeComponents;
	std::set<String> readComponents;

	for (const auto& family: system.families) {
		for (const auto& component: family.components) {
			if (component.write) {
				writeComponents.insert(component.name);
			} else {
				readComponents.insert(component.name);
			}
		}
	}

	auto systemComponents = getWidgetAs<UIWidget>("systemComponents");
	systemComponents->clear();
	auto labelStyle = factory.getStyle("label").getTextRenderer("label");
	auto readStyle = labelStyle.clone().setColour(Colour(0.2f, 1.0f, 0.2f));
	auto writeStyle = labelStyle.clone().setColour(Colour(1.0f, 0.2f, 0.2f));
	auto addLabel = [&] (const String& label, bool hasRead, bool hasWrite)
	{
		const auto style = hasWrite ? writeStyle : (hasRead ? readStyle: labelStyle);
		systemComponents->add(std::make_shared<UILabel>("", factory.getStyle("label"), style, LocalisedString::fromUserString(label)), 0, Vector4f(5, 0, 5, 0));
	};

	addLabel("[World]", false, (int(system.access) & int(SystemAccess::World)) != 0);
	addLabel("[Resources]", (int(system.access) & int(SystemAccess::Resources)) != 0, false);
	addLabel("[API]", false, (int(system.access) & int(SystemAccess::API)) != 0);
	
	for (const auto& component: ecsData.getComponents()) {
		const bool hasWrite = writeComponents.find(component.first) != writeComponents.end();
		const bool hasRead = readComponents.find(component.first) != readComponents.end();
		addLabel(component.first, hasRead, hasWrite);
	}
}

void ECSWindow::populateComponent(const String& name)
{
	const auto& ecsData = project.getECSData();
	
	auto componentSystems = getWidgetAs<UIWidget>("componentSystems");
	componentSystems->clear();
	auto labelStyle = factory.getStyle("label").getTextRenderer("label");
	auto readStyle = labelStyle.clone().setColour(Colour(0.2f, 1.0f, 0.2f));
	auto writeStyle = labelStyle.clone().setColour(Colour(1.0f, 0.2f, 0.2f));
	
	for (const auto& system: ecsData.getSystems()) {
		bool hasWrite = false;
		bool hasRead = false;
		for (const auto& family: system.second.families) {
			for (const auto& component: family.components) {
				if (component.name == name) {
					if (component.write) {
						hasWrite = true;
					} else {
						hasRead = true;
					}
				}
			}
		}
		const auto style = hasWrite ? writeStyle : (hasRead ? readStyle: labelStyle);
		componentSystems->add(std::make_shared<UILabel>("", factory.getStyle("label"), style, LocalisedString::fromUserString(system.first)), 0, Vector4f(5, 0, 5, 0));
	}
}
