#include "choose_asset_window.h"
using namespace Halley;

ChooseAssetWindow::ChooseAssetWindow(UIFactory& factory, std::function<void(std::optional<String>)> callback)
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
	getWidgetAs<UILabel>("title")->setText(getTitle());
	getRoot()->setFocus(getWidget("search"));
}

void ChooseAssetWindow::setAssetIds(const std::vector<String>& ids)
{
	for (const auto& c: ids) {
		options->addTextItem(c, LocalisedString::fromUserString(c));
	}
}

LocalisedString ChooseAssetWindow::getTitle() const
{
	return LocalisedString::fromHardcodedString("Choose Asset");
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
