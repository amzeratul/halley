#include "graphics/sprite/sprite_painter.h"
#include "graphics/sprite/sprite.h"
#include "graphics/painter.h"
#include <gsl/gsl>

using namespace Halley;

SpritePainterEntry::SpritePainterEntry(Sprite& sprite, int layer, float tieBreaker)
	: sprite(&sprite)
	, layer(layer)
	, tieBreaker(tieBreaker)
{}

SpritePainterEntry::SpritePainterEntry(size_t spriteIdx, int layer, float tieBreaker)
	: index(spriteIdx)
	, layer(layer)
	, tieBreaker(tieBreaker)
{}

bool SpritePainterEntry::operator<(const SpritePainterEntry& o) const
{
	if (layer != o.layer) {
		return layer < o.layer;
	} else if (tieBreaker != o.tieBreaker) {
		return tieBreaker < o.tieBreaker;
	} else {
		return sprite < o.sprite;
	}
}

bool SpritePainterEntry::hasSprite() const
{
	return sprite != nullptr;
}

Sprite& SpritePainterEntry::getSprite() const
{
	Expects(sprite != nullptr);
	return *sprite;
}

size_t SpritePainterEntry::getIndex() const
{
	Expects(sprite == nullptr);
	return index;
}

void SpritePainter::start(size_t nSprites)
{
	if (sprites.capacity() < nSprites) {
		sprites.reserve(nSprites);
	}
	sprites.clear();
}

void SpritePainter::add(Sprite& sprite, int layer, float tieBreaker)
{
	sprites.push_back(SpritePainterEntry(sprite, layer, tieBreaker));
}

void SpritePainter::addCopy(const Sprite& sprite, int layer, float tieBreaker)
{
	sprites.push_back(SpritePainterEntry(cached.size(), layer, tieBreaker));
	cached.push_back(sprite);
}

void SpritePainter::draw(Painter& painter)
{
	// TODO: implement hierarchical bucketing.
	// - one bucket per layer
	// - for each layer, one bucket per vertical band of the screen (32px or so)
	// - sort each leaf bucket
	std::sort(sprites.begin(), sprites.end()); // lol
	for (auto& s : sprites) {
		if (s.hasSprite()) {
			s.getSprite().draw(painter);
		} else {
			cached[s.getIndex()].draw(painter);
		}
	}
	painter.flush();
}
