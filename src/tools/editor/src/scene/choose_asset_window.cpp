#include "choose_asset_window.h"
#include "halley/ui/ui_anchor.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/widgets/ui_label.h"
#include "halley/ui/widgets/ui_list.h"

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
	getWidget("search")->focus();
	getRoot()->registerKeyPressListener(getWidget("options"), 2);
	getRoot()->registerKeyPressListener(shared_from_this(), 1);
}

void ChooseAssetWindow::setAssetIds(const std::vector<String>& ids, const String& defaultOption)
{
	if (canShowBlank()) {
		options->addTextItem("", LocalisedString::fromHardcodedString("[Empty]"));
	}
	
	for (const auto& c: ids) {
		options->addTextItem(c, LocalisedString::fromUserString(c));
	}
	options->setSelectedOptionId(defaultOption);
}

void ChooseAssetWindow::setTitle(LocalisedString title)
{
	getWidgetAs<UILabel>("title")->setText(std::move(title));
}

bool ChooseAssetWindow::onKeyPress(KeyboardKeyPress key)
{
	if (key.is(KeyCode::Enter)) {
		accept();
		return true;
	}

	if (key.is(KeyCode::Esc)) {
		cancel();
		return true;
	}

	return false;
}

bool ChooseAssetWindow::canShowBlank() const
{
	return true;
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
		if (callback) {
			callback(id);
			destroy();
		}
		callback = {};
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
	setAssetIds(componentList, "");
	setTitle(LocalisedString::fromHardcodedString("Add Component"));
}

bool AddComponentWindow::canShowBlank() const
{
	return false;
}

ChooseAssetTypeWindow::ChooseAssetTypeWindow(UIFactory& factory, AssetType type, String defaultOption, Resources& gameResources, Callback callback)
	: ChooseAssetWindow(factory, std::move(callback))
{
	setAssetIds(gameResources.ofType(type).enumerate(), defaultOption);
	setTitle(LocalisedString::fromHardcodedString("Choose " + toString(type)));
}
