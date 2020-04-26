#include "graphics/sprite/sprite_painter.h"
#include "graphics/sprite/sprite.h"
#include "graphics/painter.h"
#include <gsl/gsl>
#include "graphics/text/text_renderer.h"

using namespace Halley;

SpritePainterEntry::SpritePainterEntry(const Sprite& sprite, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip)
	: ptr(&sprite)
	, type(SpritePainterEntryType::SpriteRef)
	, layer(layer)
	, mask(mask)
	, tieBreaker(tieBreaker)
	, clip(clip)
{}

SpritePainterEntry::SpritePainterEntry(const TextRenderer& text, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip)
	: ptr(&text)
	, type(SpritePainterEntryType::TextRef)
	, layer(layer)
	, mask(mask)
	, tieBreaker(tieBreaker)
	, clip(clip)
{
}

SpritePainterEntry::SpritePainterEntry(SpritePainterEntryType type, size_t spriteIdx, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip)
	: index(static_cast<int>(spriteIdx))
	, type(type)
	, layer(layer)
	, mask(mask)
	, tieBreaker(tieBreaker)
	, clip(clip)
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

const Sprite& SpritePainterEntry::getSprite() const
{
	Expects(ptr != nullptr);
	Expects(type == SpritePainterEntryType::SpriteRef);
	return *static_cast<const Sprite*>(ptr);
}

const TextRenderer& SpritePainterEntry::getText() const
{
	Expects(ptr != nullptr);
	Expects(type == SpritePainterEntryType::TextRef);
	return *static_cast<const TextRenderer*>(ptr);
}

size_t SpritePainterEntry::getIndex() const
{
	Expects(ptr == nullptr);
	return index;
}

int SpritePainterEntry::getMask() const
{
	return mask;
}

const std::optional<Rect4f>& SpritePainterEntry::getClip() const
{
	return clip;
}

void SpritePainter::start()
{
	sprites.clear();
	cachedSprites.clear();
	cachedText.clear();
}

void SpritePainter::start(size_t)
{
	start();
}

void SpritePainter::add(const Sprite& sprite, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip)
{
	Expects(mask >= 0);
	sprites.push_back(SpritePainterEntry(sprite, mask, layer, tieBreaker, std::move(clip)));
	dirty = true;
}

void SpritePainter::addCopy(const Sprite& sprite, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip)
{
	Expects(mask >= 0);
	sprites.push_back(SpritePainterEntry(SpritePainterEntryType::SpriteCached, cachedSprites.size(), mask, layer, tieBreaker, std::move(clip)));
	cachedSprites.push_back(sprite);
	dirty = true;
}

void SpritePainter::add(const TextRenderer& text, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip)
{
	Expects(mask >= 0);
	sprites.push_back(SpritePainterEntry(text, mask, layer, tieBreaker, std::move(clip)));
	dirty = true;
}

void SpritePainter::addCopy(const TextRenderer& text, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip)
{
	Expects(mask >= 0);
	sprites.push_back(SpritePainterEntry(SpritePainterEntryType::TextCached, cachedText.size(), mask, layer, tieBreaker, std::move(clip)));
	cachedText.push_back(text);
	dirty = true;
}

void SpritePainter::draw(int mask, Painter& painter)
{
	if (dirty) {
		// TODO: implement hierarchical bucketing.
		// - one bucket per layer
		// - for each layer, one bucket per vertical band of the screen (32px or so)
		// - sort each leaf bucket
		std::sort(sprites.begin(), sprites.end()); // lol
		dirty = false;
	}

	// View
	auto& cam = painter.getCurrentCamera();
	Rect4f view = cam.getClippingRectangle();

	// Draw!
	for (auto& s : sprites) {
		if ((s.getMask() & mask) != 0) {
			const auto type = s.getType();
			
			if (type == SpritePainterEntryType::SpriteRef) {
				draw(s.getSprite(), painter, view, s.getClip());
			} else if (type == SpritePainterEntryType::SpriteCached) {
				draw(cachedSprites[s.getIndex()], painter, view, s.getClip());
			} else if (type == SpritePainterEntryType::TextRef) {
				draw(s.getText(), painter, view, s.getClip());
			} else if (type == SpritePainterEntryType::TextCached) {
				draw(cachedText[s.getIndex()], painter, view, s.getClip());
			}
		}
	}
	painter.flush();
}

void SpritePainter::draw(const Sprite& sprite, Painter& painter, Rect4f view, const std::optional<Rect4f>& clip) const
{
	if (sprite.isInView(view)) {
		sprite.draw(painter, clip);
	}
}

void SpritePainter::draw(const TextRenderer& text, Painter& painter, Rect4f view, const std::optional<Rect4f>& clip) const
{
	text.draw(painter, clip);
}
