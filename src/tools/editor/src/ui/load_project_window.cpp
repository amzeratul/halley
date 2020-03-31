#include "load_project_window.h"
#include "src/halley_editor.h"
#include "src/preferences.h"
using namespace Halley;

LoadProjectWindow::LoadProjectWindow(UIFactory& factory, HalleyEditor& editor, std::function<void(String)> callback)
	: UIWidget("load_project", {}, UISizer())
{
	UIWidget::add(factory.makeUI("ui/halley/load_project"));

	setHandle(UIEventType::ListSelectionChanged, [=] (const UIEvent& event)
	{
		event.getCurWidget().getWidgetAs<UITextInput>("input")->setText(event.getStringData());
	});

	setHandle(UIEventType::ListAccept, [=] (const UIEvent& event)
	{
		auto result = event.getCurWidget().getWidgetAs<UITextInput>("input")->getText();
		callback(result);
	});

	setHandle(UIEventType::ButtonClicked, "ok", [=] (const UIEvent& event)
	{
		auto result = event.getCurWidget().getWidgetAs<UITextInput>("input")->getText();
		callback(result);
	});

	auto recent = getWidgetAs<UIList>("recent");
	recent->setSingleClickAccept(false);
	for (auto& r: editor.getPreferences().getRecents()) {
		recent->addTextItem(r, LocalisedString::fromUserString(r));
	}
	recent->addTextItem("", LocalisedString::fromHardcodedString("New location..."));
}
