#include "widgets/ui_image.h"

using namespace Halley;

UIImage::UIImage(Sprite s, std::optional<UISizer> sizer, Vector4f innerBorder)
	: UIWidget("", {}, std::move(sizer), innerBorder)
{
	setSprite(s);
}

UIImage::UIImage(String id, Sprite s, std::optional<UISizer> sizer, Vector4f innerBorder)
	: UIWidget(std::move(id), {}, std::move(sizer), innerBorder)
{
	setSprite(s);
}

void UIImage::draw(UIPainter& painter) const
{
	if (sprite.hasMaterial()) {
		if (layerAdjustment != 0 || worldClip) {
			auto p2 = painter.withAdjustedLayer(layerAdjustment).withClip(worldClip);
			p2.draw(sprite);
		} else {
			painter.draw(sprite);
		}
	}
}

void UIImage::update(Time t, bool moved)
{
	if (moved || dirty) {
		Vector2f basePos = getPosition();
		Vector2f imgBaseSize = sprite.getSize() + topLeftBorder + bottomRightBorder;
		if (sprite.getClip()) {
			auto c = sprite.getClip().value();
			basePos -= c.getTopLeft();
			imgBaseSize = std::min(c.getSize(), imgBaseSize);
		}
		sprite
			.setPos(basePos)
			.setScale(getSize() / imgBaseSize);
		dirty = false;
	}
}

void UIImage::setSprite(Sprite s)
{
	sprite = std::move(s);

	const auto b = sprite.getOuterBorder();
	topLeftBorder = Vector2f(b.xy());
	bottomRightBorder = Vector2f(b.zw());
	const auto c = sprite.getClip();
	
	const auto spriteSize = (sprite.getSize() + topLeftBorder + bottomRightBorder) * sprite.getScale();
	//sprite.setAbsolutePivot(-topLeftBorder + sprite.getAbsolutePivot());
	
	if (c) {
		setMinSize(Vector2f::min(spriteSize, c->getSize()));
	} else {
		setMinSize(spriteSize);
	}
	dirty = true;
}

Sprite& UIImage::getSprite()
{
	return sprite;
}

const Sprite& UIImage::getSprite() const
{
	return sprite;
}

void UIImage::setLayerAdjustment(int adjustment)
{
	layerAdjustment = adjustment;
}

void UIImage::setWorldClip(std::optional<Rect4f> wc)
{
	worldClip = wc;
}

void UIImage::setSelectable(Colour4f normalColour, Colour4f selColour)
{
	setHandle(UIEventType::SetSelected, [=] (const UIEvent& event)
	{
		if (event.getBoolData()) {
			sprite.setColour(selColour);
		} else {
			sprite.setColour(normalColour);
		}
	});
}

void UIImage::setSelectable(Sprite normalSprite, Sprite selectedSprite)
{
	setHandle(UIEventType::SetSelected, [=] (const UIEvent& event)
	{
		if (event.getBoolData()) {
			sprite = selectedSprite;
		} else {
			sprite = normalSprite;
		}
		dirty = true;
	});
}

void UIImage::setDisablable(Colour4f normalColour, Colour4f disabledColour)
{
	setHandle(UIEventType::SetEnabled, [=] (const UIEvent& event)
	{
		if (event.getBoolData()) {
			sprite.setColour(normalColour);
		} else {
			sprite.setColour(disabledColour);
		}
	});
}

void UIImage::setHoverable(Colour4f normalColour, Colour4f selColour)
{
	setHandle(UIEventType::SetHovered, [=] (const UIEvent& event)
	{
		if (event.getBoolData()) {
			sprite.setColour(selColour);
		} else {
			sprite.setColour(normalColour);
		}
	});
}

void UIImage::setHoverable(Sprite normalSprite, Sprite selectedSprite)
{
	setHandle(UIEventType::SetHovered, [=] (const UIEvent& event)
	{
		if (event.getBoolData()) {
			sprite = selectedSprite;
		} else {
			sprite = normalSprite;
		}
		dirty = true;
	});
}
