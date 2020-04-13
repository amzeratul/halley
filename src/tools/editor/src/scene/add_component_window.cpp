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

void AddComponentWindow::onAddedToRoot()
{
	getRoot()->setFocus(getWidget("search"));
}

void AddComponentWindow::makeUI(const std::vector<String>& componentList)
{
	add(factory.makeUI("ui/halley/add_component_window"), 1);

	options = getWidgetAs<UIList>("options");
	for (auto& c: componentList) {
		options->addTextItem(c, LocalisedString::fromUserString(c));
	}

	setHandle(UIEventType::ButtonClicked, "ok", [=] (const UIEvent& event)
	{
		accept();
	});

	setHandle(UIEventType::ButtonClicked, "cancel", [=](const UIEvent& event)
	{
		cancel();
	});

	setHandle(UIEventType::TextChanged, "search", [=](const UIEvent& event)
	{
		setFilter(event.getStringData());
	});

	setHandle(UIEventType::TextSubmit, "search", [=](const UIEvent& event)
	{
		accept();
	});
}

void AddComponentWindow::accept()
{
	const auto id = getWidgetAs<UIList>("options")->getSelectedOptionId();
	if (!id.isEmpty()) {
		callback(id);
		destroy();
	}
}

void AddComponentWindow::cancel()
{
	destroy();
}

void AddComponentWindow::setFilter(const String& str)
{
	options->filterOptions(str);
}
