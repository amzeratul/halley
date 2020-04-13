#include "choose_asset_window.h"
using namespace Halley;

ChooseAssetWindow::ChooseAssetWindow(UIFactory& factory, Callback callback)
	: UIWidget("choose_asset_window", {}, UISizer())
	, factory(factory)
	, callback(std::move(callback))
{
	makeUI();
	setModal(true);
	setAnchor(UIAnchor());
}

ChooseAssetWindow::~ChooseAssetWindow() = default;

void ChooseAssetWindow::onAddedToRoot()
{
	getRoot()->setFocus(getWidget("search"));
}

void ChooseAssetWindow::setAssetIds(const std::vector<String>& ids)
{
	for (const auto& c: ids) {
		options->addTextItem(c, LocalisedString::fromUserString(c));
	}
}

void ChooseAssetWindow::setTitle(LocalisedString title)
{
	getWidgetAs<UILabel>("title")->setText(std::move(title));
}

void ChooseAssetWindow::makeUI()
{
	add(factory.makeUI("ui/halley/choose_asset_window"), 1);

	options = getWidgetAs<UIList>("options");

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

void ChooseAssetWindow::accept()
{
	const auto id = getWidgetAs<UIList>("options")->getSelectedOptionId();
	if (!id.isEmpty()) {
		callback(id);
		destroy();
	}
}

void ChooseAssetWindow::cancel()
{
	destroy();
}

void ChooseAssetWindow::setFilter(const String& str)
{
	options->filterOptions(str);
}

AddComponentWindow::AddComponentWindow(UIFactory& factory, const std::vector<String>& componentList, Callback callback)
	: ChooseAssetWindow(factory, std::move(callback))
{
	setAssetIds(componentList);
	setTitle(LocalisedString::fromHardcodedString("Add Component"));
}

ChoosePrefabWindow::ChoosePrefabWindow(UIFactory& factory, Resources& gameResources, Callback callback)
	: ChooseAssetWindow(factory, std::move(callback))
{
	setAssetIds(gameResources.enumerate<Prefab>());
	setTitle(LocalisedString::fromHardcodedString("Choose prefab"));
}
