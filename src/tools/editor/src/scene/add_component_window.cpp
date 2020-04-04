#include "add_component_window.h"
using namespace Halley;

AddComponentWindow::AddComponentWindow(UIFactory& factory, const std::vector<String>& componentList, std::function<void(std::optional<String>)> callback)
	: UIWidget("add_component_window", {}, UISizer())
	, factory(factory)
	, callback(std::move(callback))
{
	makeUI(componentList);
	setModal(true);
	setAnchor(UIAnchor());
}

void AddComponentWindow::makeUI(const std::vector<String>& componentList)
{
	add(factory.makeUI("ui/halley/add_component_window"), 1);

	auto options = getWidgetAs<UIList>("options");
	for (auto& c: componentList) {
		options->addTextItem(c, LocalisedString::fromUserString(c));
	}

	setHandle(UIEventType::ButtonClicked, "ok", [=] (const UIEvent& event)
	{
		callback(getWidgetAs<UIList>("options")->getSelectedOptionId());
		event.getCurWidget().destroy();
	});

	setHandle(UIEventType::ButtonClicked, "cancel", [=](const UIEvent& event)
	{
		event.getCurWidget().destroy();
	});
}
