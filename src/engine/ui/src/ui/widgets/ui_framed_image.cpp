#include "widgets/ui_framed_image.h"

using namespace Halley;

UIFramedImage::UIFramedImage(const String& id, UIStyle style, std::optional<UISizer> sizer)
	: UIFramedImage(id, style.getSprite("frame"), style.getSprite("framedImage"), style.getBorder("frameBorder"), std::move(sizer), style.getBorder("innerBorder"))
{
	styles.emplace_back(std::move(style));
}

UIFramedImage::UIFramedImage(const String& id, Sprite frame, Sprite framedImage, Vector4f frameBorder, std::optional<UISizer> sizer, Vector4f innerBorder)
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

	int xCount = int(std::ceil((rect.getRight() - startPos.x) / frameSize.x));
	int yCount = int(std::ceil((rect.getBottom() - startPos.y) / frameSize.y));

	for (int iy = 0; iy < yCount; ++iy) {
		float y0 = startPos.y + iy * frameSize.y;
		float y1 = startPos.y + (iy + 1) * frameSize.y;
		if (y1 < rect.getTop()) {
			continue;
		}
		for (int ix = 0; ix < xCount; ++ix) {
			float x0 = startPos.x + ix * frameSize.x;
			float x1 = startPos.x + (ix + 1) * frameSize.x;
			if (x1 < rect.getLeft()) {
				continue;
			}
			const Vector2f drawPos = Vector2f(x0, y0);
			const Vector2f drawSize = Vector2f(x1 - x0 + 0.25f, y1 - y0 + 0.25f);
			clippedPainter.draw(framedSprite.clone().setPos(drawPos).setSize(drawSize), true);
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

void UIFramedImage::setScrolling(Vector2f ss, std::optional<Vector2f> startPos)
{
	if (startPos) {
		scrollPos = startPos.value();
	}
	scrollSpeed = ss;
}
