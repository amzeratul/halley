#include "load_project_window.h"
#include "src/halley_editor.h"
#include "src/preferences.h"
using namespace Halley;

LoadProjectWindow::LoadProjectWindow(UIFactory& factory, HalleyEditor& editor, std::function<void(String)> callback)
	: UIWidget("load_project", {}, UISizer(UISizerType::Vertical))
	, editor(editor)
{
	UIWidget::add(factory.makeUI("ui/halley/load_project"), 1, {}, UISizerAlignFlags::CentreHorizontal | UISizerFillFlags::FillVertical);

	{
		auto logo = getWidgetAs<UIImage>("logo");
		auto col = factory.getColourScheme()->getColour("logo");
		
		auto halleyLogo = Sprite()
			.setImage(factory.getResources(), "halley/halley_logo_dist.png", "Halley/DistanceFieldSprite")
			.setColour(col)
			.setScale(Vector2f(2, 2))
			.setPos(Vector2f(640, 360));
		halleyLogo.getMutableMaterial()
			.set("u_smoothness", 20.0f)
			.set("u_outline", 0.0f)
			.set("u_outlineColour", col);

		logo->setSprite(halleyLogo);
	}

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
		auto p = Path(r).getNativeString();
		recent->addTextItem(p, LocalisedString::fromUserString(p));
	}
	recent->addTextItem("", LocalisedString::fromHardcodedString("New location..."));
	
	setAnchor(UIAnchor());
}

void LoadProjectWindow::onAddedToRoot()
{
	getRoot()->setFocus(getWidget("recent"));
}

void LoadProjectWindow::update(Time t, bool moved)
{
	const auto size = editor.getAPI().video->getWindow().getDefinition().getSize();
	setMinSize(Vector2f(size));
}
