#include "sprite_painter.h"
#include "sprite.h"

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
	std::sort(sprites.begin(), sprites.end()); // lol
	for (auto& s : sprites) {
		s.getSprite().draw(painter);
	}
}
