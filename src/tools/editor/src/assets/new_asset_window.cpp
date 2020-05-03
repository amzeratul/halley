#include "new_asset_window.h"
using namespace Halley;

NewAssetWindow::NewAssetWindow(UIFactory& factory, Callback callback)
	: UIWidget("new_asset", Vector2f(), UISizer())
	, factory(factory)
	, callback(std::move(callback))
{
	makeUI();
	setModal(true);
	setAnchor(UIAnchor());
}

void NewAssetWindow::onAddedToRoot()
{
	getWidget("name")->focus();
}

void NewAssetWindow::makeUI()
{
	add(factory.makeUI("ui/halley/new_asset_window"));

	const auto name = getWidgetAs<UITextInput>("name");
	getWidget("ok")->setEnabled(false);

	setHandle(UIEventType::ButtonClicked, "ok", [=] (const UIEvent& event)
	{
		callback(name->getText().isEmpty() ? std::optional<String>() : String(name->getText()));
		destroy();
	});
	
	setHandle(UIEventType::ButtonClicked, "cancel", [=] (const UIEvent& event)
	{
		callback({});
		destroy();
	});

	setHandle(UIEventType::TextChanged, "name", [=] (const UIEvent& event)
	{
		getWidget("ok")->setEnabled(!event.getStringData().isEmpty());
	});

	setHandle(UIEventType::TextSubmit, "name", [=] (const UIEvent& event)
	{
		callback(event.getStringData().isEmpty() ? std::optional<String>() : event.getStringData());
		destroy();
	});
}
