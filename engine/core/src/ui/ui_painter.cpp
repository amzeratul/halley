#include "ui/ui_painter.h"
#include "graphics/sprite/sprite_painter.h"
#include "graphics/sprite/sprite.h"
#include "graphics/text/text_renderer.h"
using namespace Halley;

UIPainter::UIPainter(SpritePainter& painter, int mask, int layer)
	: painter(painter)
	, mask(mask)
	, layer(layer)
	, n(0)
{
}

UIPainter UIPainter::clone() const
{
	auto result = UIPainter(painter, mask, layer);
	result.n = n;
	return result;
}

UIPainter UIPainter::withAdjustedLayer(int delta) const
{
	auto result = clone();
	result.layer += delta;
	return result;
}

UIPainter UIPainter::withClip(Rect4f newClip) const
{
	auto result = clone();
	if (clip) {
		result.clip = clip.get().intersection(newClip);
	} else {
		result.clip = newClip;
	}
	return result;
}

void UIPainter::draw(const Sprite& sprite)
{
	if (clip) {
		auto targetClip = clip.get() - sprite.getPosition();
		if (sprite.getClip()) {
			targetClip = sprite.getClip().get().intersection(targetClip);
		}

		auto onScreen = sprite.getAABB().intersection(targetClip + sprite.getPosition());
		if (onScreen.getWidth() > 0.1f && onScreen.getHeight() > 0.1f) {
			painter.addCopy(sprite.clone().setClip(targetClip), mask, layer, float(n++));
		}
	} else {
		painter.add(sprite, mask, layer, float(n++));
	}
}

void UIPainter::draw(const TextRenderer& text)
{
	if (clip) {
		auto targetClip = clip.get() - text.getPosition();
		if (text.getClip()) {
			targetClip = text.getClip().get().intersection(targetClip);
		}
		
		auto onScreen = Rect4f(Vector2f(), text.getExtents()).intersection(targetClip);
		if (onScreen.getWidth() > 0.1f && onScreen.getHeight() > 0.1f) {
			painter.addCopy(text.clone().setClip(clip.get() - text.getPosition()), mask, layer, float(n++));
		}
	} else {
		painter.add(text, mask, layer, float(n++));
	}
}
