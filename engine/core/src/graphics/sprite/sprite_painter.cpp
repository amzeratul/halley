#include "graphics/sprite/sprite_painter.h"
#include "graphics/sprite/sprite.h"
#include "graphics/painter.h"
#include <gsl/gsl>
#include <boost/asio/detail/buffer_sequence_adapter.hpp>
#include "graphics/text/text_renderer.h"

using namespace Halley;

SpritePainterEntry::SpritePainterEntry(Sprite& sprite, int layer, float tieBreaker)
	: ptr(&sprite)
	, type(SpritePainterEntryType::SpriteRef)
	, layer(layer)
	, tieBreaker(tieBreaker)
{}

SpritePainterEntry::SpritePainterEntry(TextRenderer& text, int layer, float tieBreaker)
	: ptr(&text)
	, type(SpritePainterEntryType::TextRef)
	, layer(layer)
	, tieBreaker(tieBreaker)
{
}

SpritePainterEntry::SpritePainterEntry(SpritePainterEntryType type, size_t spriteIdx, int layer, float tieBreaker)
	: index(int(spriteIdx))
	, type(type)
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
		return ptr < o.ptr;
	}
}

SpritePainterEntryType SpritePainterEntry::getType() const
{
	return type;
}

Sprite& SpritePainterEntry::getSprite() const
{
	Expects(ptr != nullptr);
	Expects(type == SpritePainterEntryType::SpriteRef);
	return *reinterpret_cast<Sprite*>(ptr);
}

TextRenderer& SpritePainterEntry::getText() const
{
	Expects(ptr != nullptr);
	Expects(type == SpritePainterEntryType::TextRef);
	return *reinterpret_cast<TextRenderer*>(ptr);
}

size_t SpritePainterEntry::getIndex() const
{
	Expects(ptr == nullptr);
	return index;
}

void SpritePainter::start(size_t nSprites)
{
	if (sprites.capacity() < nSprites) {
		sprites.reserve(nSprites);
	}
	sprites.clear();
	cachedSprites.clear();
}

void SpritePainter::add(Sprite& sprite, int layer, float tieBreaker)
{
	sprites.push_back(SpritePainterEntry(sprite, layer, tieBreaker));
}

void SpritePainter::addCopy(const Sprite& sprite, int layer, float tieBreaker)
{
	sprites.push_back(SpritePainterEntry(SpritePainterEntryType::SpriteCached, cachedSprites.size(), layer, tieBreaker));
	cachedSprites.push_back(sprite);
}

void SpritePainter::add(TextRenderer& text, int layer, float tieBreaker)
{
	sprites.push_back(SpritePainterEntry(text, layer, tieBreaker));
}

void SpritePainter::addCopy(const TextRenderer& text, int layer, float tieBreaker)
{
	sprites.push_back(SpritePainterEntry(SpritePainterEntryType::TextCached, cachedText.size(), layer, tieBreaker));
	cachedText.push_back(text);
}

void SpritePainter::draw(Painter& painter)
{
	// TODO: implement hierarchical bucketing.
	// - one bucket per layer
	// - for each layer, one bucket per vertical band of the screen (32px or so)
	// - sort each leaf bucket
	std::sort(sprites.begin(), sprites.end()); // lol

	// Draw!
	for (auto& s : sprites) {
		auto type = s.getType();
		if (type == SpritePainterEntryType::SpriteRef) {
			s.getSprite().draw(painter);
		} else if (type == SpritePainterEntryType::SpriteCached) {
			cachedSprites[s.getIndex()].draw(painter);
		} else if (type == SpritePainterEntryType::TextRef) {
			s.getText().draw(painter);
		} else if (type == SpritePainterEntryType::TextCached) {
			cachedText[s.getIndex()].draw(painter);
		}
	}
	painter.flush();
}
