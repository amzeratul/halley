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

void UIPainter::draw(const Sprite& sprite, int layerOffset)
{
	painter.add(sprite, mask, layer + layerOffset, float(n++));
}

void UIPainter::draw(const TextRenderer& sprite, int layerOffset)
{
	painter.add(sprite, mask, layer + layerOffset, float(n++));
}