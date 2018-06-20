#include "widgets/ui_image.h"

using namespace Halley;

UIImage::UIImage(Sprite s, Maybe<UISizer> sizer, Vector4f innerBorder)
	: UIWidget("", {}, std::move(sizer), innerBorder)
{
	setSprite(s);
}

UIImage::UIImage(const String& id, Sprite s, Maybe<UISizer> sizer, Vector4f innerBorder)
	: UIWidget(id, {}, std::move(sizer), innerBorder)
{
	setSprite(s);
}

void UIImage::draw(UIPainter& painter) const
{
	if (sprite.hasMaterial()) {
		if (layerAdjustment != 0) {
			auto p2 = painter.withAdjustedLayer(layerAdjustment);
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
		Vector2f imgBaseSize = sprite.getRawSize().abs() + topLeftBorder + bottomRightBorder;
		if (sprite.getClip()) {
			auto c = sprite.getClip().get();
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
	sprite = s;

	auto b = sprite.getOuterBorder();
	topLeftBorder = Vector2f(float(b.x), float(b.y));
	bottomRightBorder = Vector2f(float(b.z), float(b.w));
	auto c = s.getClip();
	
	auto spriteSize = (s.getRawSize().abs() + topLeftBorder + bottomRightBorder) * s.getScale();
	sprite.setAbsolutePivot(-topLeftBorder + sprite.getAbsolutePivot());
	
	if (c) {
		setMinSize(Vector2f::min(spriteSize, c.get().getSize()));
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
