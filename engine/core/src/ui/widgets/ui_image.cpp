#include "ui/widgets/ui_image.h"

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
		sprite.setPos(getPosition()).scaleTo(getSize());
		dirty = false;
	}
}

void UIImage::setSprite(Sprite s)
{
	sprite = s;
	auto c = s.getClip();
	if (c) {
		setMinSize(Vector2f::min(s.getScaledSize().abs(), c.get().getSize()));
	} else {
		setMinSize(s.getScaledSize().abs());
	}
	dirty = true;
}
