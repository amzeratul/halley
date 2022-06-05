#include "colour_picker.h"
using namespace Halley;

ColourPickerButton::ColourPickerButton(UIFactory& factory, Colour4f colour, Callback callback)
	: UIImage(Sprite().setImage(factory.getResources(), "halley_ui/ui_list_item.png").setColour(colour))
	, factory(factory)
	, callback(std::move(callback))
{
	setMinSize(Vector2f(40, 22));
	setInteractWithMouse(true);
}

void ColourPickerButton::setColour(Colour4f colour, bool final)
{
	getSprite().setColour(colour);
	if (callback) {
		callback(colour, final);
	}
}

void ColourPickerButton::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	if (button == 0 && keyMods == KeyMods::None) {
		getRoot()->addChild(std::make_shared<ColourPicker>(factory, getSprite().getColour(), callback));
	}
}

ColourPicker::ColourPicker(UIFactory& factory, Colour4f initialColour, Callback callback)
	: UIWidget("colourPicker", {}, UISizer())
	, initialColour(initialColour)
	, colour(initialColour)
	, callback(std::move(callback))
{
	factory.loadUI(*this, "halley/colour_picker");
	setModal(true);
	setAnchor(UIAnchor());
}

Colour4f ColourPicker::getColour() const
{
	return colour;
}

void ColourPicker::onMakeUI()
{
	setHandle(UIEventType::ButtonClicked, "ok", [=] (const UIEvent& event)
	{
		accept();
	});

	setHandle(UIEventType::ButtonClicked, "cancel", [=] (const UIEvent& event)
	{
		cancel();
	});
}

void ColourPicker::accept()
{
	if (callback) {
		callback(colour, true);
	}
	destroy();
}

void ColourPicker::cancel()
{
	if (callback) {
		callback(initialColour, false);
	}
	destroy();
}

void ColourPicker::onColourChanged()
{
	if (callback) {
		callback(colour, false);
	}
}
