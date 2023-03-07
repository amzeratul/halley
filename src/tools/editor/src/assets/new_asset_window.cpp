#include "new_asset_window.h"
using namespace Halley;

NewAssetWindow::NewAssetWindow(UIFactory& factory, LocalisedString label, String startValue, Callback callback)
	: UIWidget("new_asset", Vector2f(), UISizer())
	, factory(factory)
	, label(std::move(label))
	, startValue(std::move(startValue))
	, callback(std::move(callback))
{
	makeUI();
	setModal(true);
	setAnchor(UIAnchor());
	setChildLayerAdjustment(10);
}

void NewAssetWindow::onAddedToRoot(UIRoot& root)
{
	root.setFocus(getWidget("name"));
}

void NewAssetWindow::makeUI()
{
	add(factory.makeUI("halley/new_asset_window"));

	getWidgetAs<UILabel>("title")->setText(label);

	const auto name = getWidgetAs<UITextInput>("name");
	name->setText(startValue);
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
