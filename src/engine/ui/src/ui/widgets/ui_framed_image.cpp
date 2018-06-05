#include "widgets/ui_framed_image.h"
using namespace Halley;

UIFramedImage::UIFramedImage(const String& id, UIStyle style, Maybe<UISizer> sizer)
	: UIFramedImage(id, style.getSprite("frame"), style.getSprite("framedImage"), style.getBorder("frameBorder"), std::move(sizer), style.getBorder("innerBorder"))
{
}

UIFramedImage::UIFramedImage(const String& id, Sprite frame, Sprite framedImage, Vector4f frameBorder, Maybe<UISizer> sizer, Vector4f innerBorder)
	: UIImage(id, frame, std::move(sizer), innerBorder)
	, framedSprite(framedImage)
	, frameBorder(frameBorder)
{
}

void UIFramedImage::draw(UIPainter& painter) const
{
	auto rect = Rect4f(getPosition() + Vector2f(frameBorder.x, frameBorder.y), getPosition() + getSize() - Vector2f(frameBorder.z, frameBorder.w));
	auto clippedPainter = painter.withClip(rect);

	const auto frameSize = framedSprite.getSize();	
	const auto startPos = rect.getTopLeft() + scrollPos - frameSize;
	for (float y = startPos.y; y < rect.getBottom(); y += frameSize.y) {
		if (y + frameSize.y < rect.getTop()) {
			continue;
		}
		for (float x = startPos.x; x < rect.getRight(); x += frameSize.x) {
			if (x + frameSize.x < rect.getLeft()) {
				continue;
			}
			clippedPainter.draw(framedSprite.clone().setPos(Vector2f(x, y)), true);
		}
	}
	
	UIImage::draw(painter);
}

void UIFramedImage::update(Time t, bool moved)
{
	const auto bgSize = framedSprite.getSize();
	scrollPos = (scrollPos + float(t) * scrollSpeed).modulo(bgSize);
	UIImage::update(t, moved);
}

void UIFramedImage::setFramedSprite(const Sprite& sprite)
{
	framedSprite = sprite;
}

Sprite& UIFramedImage::getFramedSprite()
{
	return framedSprite;
}

const Sprite& UIFramedImage::getFramedSprite() const
{
	return framedSprite;
}

void UIFramedImage::setScrolling(Vector2f ss, Maybe<Vector2f> startPos)
{
	if (startPos) {
		scrollPos = startPos.get();
	}
	scrollSpeed = ss;
}
