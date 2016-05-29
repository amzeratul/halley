#include "graphics/sprite/sprite_painter.h"
#include "graphics/sprite/sprite.h"

using namespace Halley;

SpritePainterEntry::SpritePainterEntry(Sprite& sprite, int layer, int tieBreaker)
	: sprite(&sprite)
	, layer(layer)
	, tieBreaker(tieBreaker)
{}

void SpritePainter::start(size_t nSprites)
{
	if (sprites.capacity() < nSprites) {
		sprites.reserve(nSprites);
	}
	sprites.clear();
}

void SpritePainter::add(Sprite& sprite, int layer, int tieBreaker)
{
	sprites.push_back(SpritePainterEntry(sprite, layer, tieBreaker));
}

void SpritePainter::draw(Painter& painter)
{
	// TODO: implement hierarchical bucketing.
	// - one bucket per layer
	// - for each layer, one bucket per vertical band of the screen (32px or so)
	// - sort each leaf bucket
	std::sort(sprites.begin(), sprites.end()); // lol
	for (auto& s : sprites) {
		s.getSprite().draw(painter);
	}
}
