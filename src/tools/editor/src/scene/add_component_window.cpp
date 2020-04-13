#include "add_component_window.h"
using namespace Halley;

AddComponentWindow::AddComponentWindow(UIFactory& factory, const std::vector<String>& componentList, std::function<void(std::optional<String>)> callback)
	: ChooseAssetWindow(factory, std::move(callback))
{
	setAssetIds(componentList);
}

LocalisedString AddComponentWindow::getTitle() const
{
	return LocalisedString::fromHardcodedString("Add Component");
}
