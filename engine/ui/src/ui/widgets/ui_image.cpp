#include "widgets/ui_image.h"

using namespace Halley;

UIImage::UIImage(Sprite s, Maybe<UISizer> sizer, Vector4f innerBorder)
	: UIWidget("", {}, sizer, innerBorder)
{
	setSprite(s);
}

void UIImage::draw(UIPainter& painter) const
{
	if (sprite.hasMaterial()) {
		painter.draw(sprite);
	}
}

void UIImage::update(Time t, bool moved)
{
	if (moved || dirty) {
		auto b = sprite.getOuterBorder();
		auto tl = Vector2f(float(b.x), float(b.y));
		auto br = Vector2f(float(b.z), float(b.w));
		sprite.setPos(getPosition()).setScale(getSize() / (sprite.getRawSize() + tl + br));
		dirty = false;
	}
}

void UIImage::setSprite(Sprite s)
{
	sprite = s;

	auto b = sprite.getOuterBorder();
	auto tl = Vector2f(float(b.x), float(b.y));
	auto br = Vector2f(float(b.z), float(b.w));
	auto c = s.getClip();
	
	auto spriteSize = (s.getRawSize().abs() + tl + br) * s.getScale();
	
	if (c) {
		setMinSize(Vector2f::min(spriteSize, c.get().getSize()));
	} else {
		setMinSize(spriteSize);
	}
	dirty = true;
}
