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

	mainDisplay = getWidgetAs<ColourPickerDisplay>("mainDisplay");
	ribbonDisplay = getWidgetAs<ColourPickerDisplay>("ribbonDisplay");
}

Colour4f ColourPicker::getColour() const
{
	return colour;
}

void ColourPicker::setColour(Colour4f col)
{
	if (col != colour) {
		colour = col;
		onColourChanged();
	}
}

void ColourPicker::update(Time t, bool moved)
{
	const float h = 1 - ribbonDisplay->getValue().y;
	mainDisplay->getSprite().setCustom0(Vector4f(h, 0, 0, 0));
	const Vector2f sv = mainDisplay->getValue();
	setColour(Colour4f::fromHSV(h, sv.x, 1 - sv.y));
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

ColourPickerDisplay::ColourPickerDisplay(String id, Vector2f size, Resources& resources, const String& material)
	: UIImage(std::move(id), makeSprite(resources, size, material))
{
	setInteractWithMouse(true);
}

void ColourPickerDisplay::setValue(Vector2f value)
{
	this->value = value;
}

Vector2f ColourPickerDisplay::getValue() const
{
	return value;
}

void ColourPickerDisplay::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	if (button == 0) {
		focus();
		held = true;
		onMouseOver(mousePos);
	}
}

void ColourPickerDisplay::releaseMouse(Vector2f mousePos, int button)
{
	if (button == 0) {
		held = false;
	}
}

void ColourPickerDisplay::onMouseOver(Vector2f mousePos)
{
	if (held) {
		value = (mousePos - getPosition()) / getSize();
		value.x = clamp(value.x, 0.0f, 1.0f);
		value.y = clamp(value.y, 0.0f, 1.0f);
	}
}

bool ColourPickerDisplay::isFocusLocked() const
{
	return held;
}

Sprite ColourPickerDisplay::makeSprite(Resources& resources, Vector2f size, const String& material)
{
	return Sprite()
		.setSize(size)
		.setMaterial(resources, material)
		.setTexRect(Rect4f(Vector2f(0, 0), Vector2f(1, 1)));
}
