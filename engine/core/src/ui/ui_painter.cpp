#include "ui/ui_painter.h"
#include "graphics/sprite/sprite_painter.h"
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

UIPainter UIPainter::withClip(Rect4i clip) const
{
	auto result = clone();
	if (result.clip) {
		auto prev = result.clip.get();
		auto size = Vector2i::min(prev.getSize(), clip.getSize());
		result.clip = Rect4i(prev.getTopLeft() + clip.getTopLeft(), size.x, size.y);
	} else {
		result.clip = clip;
	}
	return result;
}

void UIPainter::draw(const Sprite& sprite)
{
	painter.add(sprite, mask, layer, float(n++));
}

void UIPainter::draw(const TextRenderer& sprite)
{
	painter.add(sprite, mask, layer, float(n++));
}
