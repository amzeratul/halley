#include "colour_picker.h"
using namespace Halley;

ColourPickerButton::ColourPickerButton(UIFactory& factory, Colour4f colour, Callback callback)
	: UIImage(Sprite().setImage(factory.getResources(), "halley_ui/ui_list_item.png").setColour(colour))
	, factory(factory)
	, callback(std::move(callback))
	, colour(colour)
{
	setMinSize(Vector2f(40, 22));
	setInteractWithMouse(true);

	label = TextRenderer()
		.setFont(factory.getResources().get<Font>("Ubuntu Regular"))
		.setSize(12)
		.setColour(Colour4f(1, 1, 1, 1))
		.setOffset(Vector2f(0.5f, 0.5f));
}

void ColourPickerButton::update(Time t, bool moved)
{
	UIImage::update(t, moved);

	const auto hsv = colour.toHSV();
	const bool isBrightCol = (Vector2f(0, 1) - hsv.yz()).length() < 0.5f;

	String labelStr = colour.withAlpha(1).toString();
	if (colour.a < 0.9999f) {
		labelStr += " [" + toString(lroundl(colour.a * 100)) + "%]";
	}

	label
		.setText(labelStr)
		.setPosition(getRect().getCenter())
		.setColour(isBrightCol ? Colour4f(0, 0, 0, 1) : Colour4f(1, 1, 1, 1));
}

void ColourPickerButton::draw(UIPainter& painter) const
{
	UIImage::draw(painter);
	painter.draw(label);
}

void ColourPickerButton::setColour(Colour4f colour, bool final)
{
	this->colour = colour;
	getSprite().setColour(colour);
	if (callback) {
		callback(colour, final);
	}
}

void ColourPickerButton::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	if (button == 0 && keyMods == KeyMods::None) {
		getRoot()->addChild(std::make_shared<ColourPicker>(factory, getSprite().getColour(), [=] (Colour4f col, bool final)
		{
			setColour(col, final);
		}));
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

	const auto hsv = colour.toHSV();

	mainDisplay = getWidgetAs<ColourPickerDisplay>("mainDisplay");
	ribbonDisplay = getWidgetAs<ColourPickerDisplay>("ribbonDisplay");
	ribbonDisplay->setCursorType(ColourPickerDisplay::CursorType::HorizontalLine);
	mainDisplay->setCallback([=](Vector2f sv)
	{
		updatingDisplay = true;
		const float h = 1 - ribbonDisplay->getValue().y;
		setColour(Colour4f::fromHSV(h, sv.x, 1 - sv.y, colour.a));
		updatingDisplay = false;
	});
	ribbonDisplay->setCallback([=](Vector2f val)
	{
		updatingDisplay = true;
		const float h = 1 - val.y;
		const Vector2f sv = mainDisplay->getValue();
		setColour(Colour4f::fromHSV(h, sv.x, 1 - sv.y, colour.a));
		mainDisplay->getSprite().setCustom0(Vector4f(h, 0, 0, 0));
		updatingDisplay = false;
	});

	colourView = getWidgetAs<UIImage>("colour");
	prevColourView = getWidgetAs<UIImage>("prevColour");

	hexCode = getWidgetAs<UITextInput>("hexCode");
	floatCode = getWidgetAs<UITextInput>("floatCode");

	rgbhsvSliders[0] = getWidgetAs<UISlider>("rSlider");
	rgbhsvSliders[1] = getWidgetAs<UISlider>("gSlider");
	rgbhsvSliders[2] = getWidgetAs<UISlider>("bSlider");
	rgbhsvSliders[3] = getWidgetAs<UISlider>("hSlider");
	rgbhsvSliders[4] = getWidgetAs<UISlider>("sSlider");
	rgbhsvSliders[5] = getWidgetAs<UISlider>("vSlider");
	alphaSlider = getWidgetAs<UISlider>("aSlider");

	bindData("hexCode", colour.toString(), [=](String val)
	{
		if (!updatingUI) {
			updatingHex = true;
			setColour(Colour4f::fromString(val));
			updatingHex = false;
		}
	});

	bindData("rSlider", colour.r * 255, [=](float val)
	{
		if (!updatingUI) {
			auto c = colour;
			c.r = val / 255.0f;
			setColour(c);
		}
	});

	bindData("gSlider", colour.g * 255, [=](float val)
	{
		if (!updatingUI) {
			auto c = colour;
			c.g = val / 255.0f;
			setColour(c);
		}
	});

	bindData("bSlider", colour.b * 255, [=](float val)
	{
		if (!updatingUI) {
			auto c = colour;
			c.b = val / 255.0f;
			setColour(c);
		}
	});

	bindData("aSlider", colour.a * 100, [=](float val)
	{
		if (!updatingUI) {
			auto c = colour;
			c.a = val / 100.0f;
			setColour(c);
		}
	});

	bindData("hSlider", hsv.x * 360.0f, [=](float val)
	{
		if (!updatingUI) {
			ribbonDisplay->setValue(Vector2f(0, 1 - val / 360.0f));
		}
	});

	bindData("sSlider", hsv.y * 255.0f, [=](float val)
	{
		if (!updatingUI) {
			auto v = mainDisplay->getValue();
			v.x = val / 255.0f;
			mainDisplay->setValue(v);
		}
	});

	bindData("vSlider", hsv.z * 255.0f, [=](float val)
	{
		if (!updatingUI) {
			auto v = mainDisplay->getValue();
			v.y = 1.0f - (val / 255.0f);
			mainDisplay->setValue(v);
		}
	});

	updateUI();
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
		if (!updatingUI) {
			updateUI();
		}
	}
}

void ColourPicker::update(Time t, bool moved)
{
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

void ColourPicker::updateUI()
{
	updatingUI = true;

	const auto startHue = 1 - ribbonDisplay->getValue().y;
	const auto startSaturation = mainDisplay->getValue().x;
	const auto hsv = colour.toHSV(startHue, startSaturation);

	if (!updatingDisplay) {
		mainDisplay->getSprite().setCustom0(Vector4f(hsv.x, 0, 0, 0));
		mainDisplay->setValue(Vector2f(hsv.y, 1 - hsv.z));
		ribbonDisplay->setValue(Vector2f(0, 1 - hsv.x));
	}

	rgbhsvSliders[0]->setValue(colour.r * 255);
	rgbhsvSliders[1]->setValue(colour.g * 255);
	rgbhsvSliders[2]->setValue(colour.b * 255);
	alphaSlider->setValue(colour.a * 100);
	rgbhsvSliders[3]->setValue(hsv.x * 360);
	rgbhsvSliders[4]->setValue(hsv.y * 255);
	rgbhsvSliders[5]->setValue(hsv.z * 255);

	colourView->getSprite().setColour(colour);
	prevColourView->getSprite().setColour(initialColour);

	if (!updatingHex) {
		hexCode->setText(colour.toString());
	}
	floatCode->setText(Vector4f(colour.r, colour.g, colour.b, colour.a).toString(3));

	updatingUI = false;
}

ColourPickerDisplay::ColourPickerDisplay(String id, Vector2f size, Resources& resources, const String& material)
	: UIImage(std::move(id), makeSprite(resources, size, material))
	, resources(resources)
{
	setInteractWithMouse(true);
}

void ColourPickerDisplay::update(Time t, bool moved)
{
	UIImage::update(t, moved);

	cursor.setPosition(getPosition() + getSize() * Vector2f(0.5f, value.y));
}

void ColourPickerDisplay::draw(UIPainter& painter) const
{
	UIImage::draw(painter);

	if (cursorType == CursorType::Circle) {
		painter.withClip(getRect()).draw([=](Painter& p)
		{
			const auto col = value.length() < 0.5f ? Colour4f(0, 0, 0, 1) : Colour4f(1, 1, 1, 1);
			p.drawCircle(value * getSize() + getPosition(), 5, 1, col);
		});
	} else if (cursorType == CursorType::HorizontalLine) {
		painter.draw(cursor);
	}
}

void ColourPickerDisplay::setCallback(Callback callback)
{
	this->callback = std::move(callback);
}

void ColourPickerDisplay::setValue(Vector2f value)
{
	this->value = value;
	if (callback) {
		callback(value);
	}
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
		auto val = (mousePos - getPosition()) / getSize();
		val.x = clamp(val.x, 0.0f, 1.0f);
		val.y = clamp(val.y, 0.0f, 1.0f);
		setValue(val);
	}
}

bool ColourPickerDisplay::isFocusLocked() const
{
	return held;
}

void ColourPickerDisplay::setCursorType(CursorType type)
{
	cursorType = type;
	if (cursorType == CursorType::HorizontalLine) {
		cursor = Sprite().setImage(resources, "halley_ui/colour_pick_ribbon_cursor.png");
	}
}

Sprite ColourPickerDisplay::makeSprite(Resources& resources, Vector2f size, const String& material)
{
	return Sprite()
		.setSize(size)
		.setMaterial(resources, material)
		.setTexRect(Rect4f(Vector2f(0, 0), Vector2f(1, 1)));
}
