#include "halley/graphics/sprite/sprite_painter.h"
#include "halley/graphics/sprite/sprite.h"
#include "halley/graphics/painter.h"
#include <gsl/gsl>

#include "halley/graphics/material/material.h"
#include "halley/graphics/material/material_definition.h"
#include "halley/graphics/text/text_renderer.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

SpritePainterEntry::SpritePainterEntry(gsl::span<const Sprite> sprites, int mask, int layer, float tieBreaker, size_t insertOrder, std::optional<Rect4f> clip)
	: ptr(sprites.empty() ? nullptr : &sprites[0])
	, count(uint32_t(sprites.size()))
	, type(SpritePainterEntryType::SpriteRef)
	, layer(layer)
	, mask(mask)
	, tieBreaker(tieBreaker)
	, insertOrder(insertOrder)
	, clip(clip)
{}

SpritePainterEntry::SpritePainterEntry(gsl::span<const TextRenderer> texts, int mask, int layer, float tieBreaker, size_t insertOrder, std::optional<Rect4f> clip)
	: ptr(texts.empty() ? nullptr : &texts[0])
	, count(uint32_t(texts.size()))
	, type(SpritePainterEntryType::TextRef)
	, layer(layer)
	, mask(mask)
	, tieBreaker(tieBreaker)
	, insertOrder(insertOrder)
	, clip(clip)
{
}

SpritePainterEntry::SpritePainterEntry(SpritePainterEntryType type, size_t spriteIdx, size_t count, int mask, int layer, float tieBreaker, size_t insertOrder, std::optional<Rect4f> clip)
	: count(uint32_t(count))
	, index(static_cast<int>(spriteIdx))
	, type(type)
	, layer(layer)
	, mask(mask)
	, tieBreaker(tieBreaker)
	, insertOrder(insertOrder)
	, clip(clip)
{}

bool SpritePainterEntry::operator<(const SpritePainterEntry& o) const
{
	if (layer != o.layer) {
		return layer < o.layer;
	} else if (tieBreaker != o.tieBreaker) {
		return tieBreaker < o.tieBreaker;
	} else {
		return insertOrder < o.insertOrder;
	}
}

SpritePainterEntryType SpritePainterEntry::getType() const
{
	return type;
}

gsl::span<const Sprite> SpritePainterEntry::getSprites() const
{
	Expects(ptr != nullptr);
	Expects(type == SpritePainterEntryType::SpriteRef);
	return gsl::span<const Sprite>(static_cast<const Sprite*>(ptr), count);
}

gsl::span<const TextRenderer> SpritePainterEntry::getTexts() const
{
	Expects(ptr != nullptr);
	Expects(type == SpritePainterEntryType::TextRef);
	return gsl::span<const TextRenderer>(static_cast<const TextRenderer*>(ptr), count);
}

uint32_t SpritePainterEntry::getIndex() const
{
	Expects(ptr == nullptr);
	return index;
}

uint32_t SpritePainterEntry::getCount() const
{
	return count;
}

int SpritePainterEntry::getMask() const
{
	return mask;
}

const std::optional<Rect4f>& SpritePainterEntry::getClip() const
{
	return clip;
}

SpritePainterMaterialParamUpdater::SpritePainterMaterialParamUpdater()
{
	setHandle("halley.texSize", [this] (MaterialUpdater& material, const String& uniformName, const String& autoVarArgs)
	{
		const int idx = autoVarArgs.toInteger();
		const auto tex = material.getTexture(idx);
		material.set(uniformName, tex ? Vector2f(tex->getSize()) : Vector2f());
	});

	setHandle("halley.texBPP", [this] (MaterialUpdater& material, const String& uniformName, const String& autoVarArgs)
	{
		const int idx = autoVarArgs.toInteger();
		const auto tex = material.getTexture(idx);
		material.set(uniformName, tex ? TextureDescriptor::getBytesPerPixel(tex->getDescriptor().format) : 1);
	});

	setHandle("halley.timeLoop", [this] (MaterialUpdater& material, const String& uniformName, const String& autoVarArgs)
	{
		const Time cycleLen = autoVarArgs.toDouble();
		const auto value = std::fmod(curTime / cycleLen, 1.0);
		material.set(uniformName, static_cast<float>(value));
	});
}

void SpritePainterMaterialParamUpdater::update(Time t)
{
	curTime += t;
}

void SpritePainterMaterialParamUpdater::copyPrevious(const SpritePainterMaterialParamUpdater& previous)
{
	curTime = previous.curTime;
}

void SpritePainterMaterialParamUpdater::setHandle(String id, Callback callback)
{
	handles[id] = std::move(callback);
}

void SpritePainterMaterialParamUpdater::preProcessMaterial(Sprite& sprite) const
{
	std::optional<MaterialUpdater> mat;

	const auto& blocks = sprite.getMaterial().getDefinition().getUniformBlocks();
	for (const auto& block: blocks) {
		for (const auto& uniform: block.uniforms) {
			const auto& var = uniform.autoVariable;
			if (!var.isEmpty()) {
				const auto split = var.split(":");
				const auto iter = handles.find(split[0]);
				if (iter != handles.end()) {
					if (!mat) {
						mat = sprite.getMutableMaterial();
					}
					iter->second(*mat, uniform.name, split.size() >= 2 ? split[1] : "");
				}
			}
		}
	}
}

bool SpritePainterMaterialParamUpdater::needsToPreProcessessMaterial(const Sprite& sprite) const
{
	return sprite.hasMaterial() && sprite.getMaterial().getDefinition().hasAutoVariables();
}

void SpritePainter::update(Time t)
{
	paramUpdater.update(t);
}

void SpritePainter::copyPrevious(const SpritePainter& prev)
{
	paramUpdater.copyPrevious(prev.paramUpdater);
}

void SpritePainter::start(bool forceCopy)
{
	this->forceCopy = forceCopy;
	sprites.clear();
	cachedSprites.clear();
	cachedText.clear();
}

void SpritePainter::add(const Sprite& sprite, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip)
{
	if (forceCopy) {
		addCopy(sprite, mask, layer, tieBreaker, clip);
	} else {
		Expects(mask >= 0);
		sprites.push_back(SpritePainterEntry(gsl::span<const Sprite>(&sprite, 1), mask, layer, tieBreaker, sprites.size(), std::move(clip)));
		dirty = true;
	}
}

void SpritePainter::add(Sprite&& sprite, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip)
{
	Expects(mask >= 0);
	sprites.push_back(SpritePainterEntry(SpritePainterEntryType::SpriteCached, cachedSprites.size(), 1, mask, layer, tieBreaker, sprites.size(), std::move(clip)));
	cachedSprites.push_back(sprite.clone(false));
	dirty = true;
}

void SpritePainter::addCopy(const Sprite& sprite, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip)
{
	Expects(mask >= 0);
	sprites.push_back(SpritePainterEntry(SpritePainterEntryType::SpriteCached, cachedSprites.size(), 1, mask, layer, tieBreaker, sprites.size(), std::move(clip)));
	cachedSprites.push_back(sprite.clone(false));
	dirty = true;
}

void SpritePainter::add(gsl::span<const Sprite> sprites, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip)
{
	if (!sprites.empty()) {
		if (forceCopy) {
			addCopy(sprites, mask, layer, tieBreaker, clip);
		} else {
			Expects(mask >= 0);
			this->sprites.push_back(SpritePainterEntry(sprites, mask, layer, tieBreaker, this->sprites.size(), std::move(clip)));
			dirty = true;
		}
	}
}

void SpritePainter::addCopy(gsl::span<const Sprite> sprites, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip)
{
	Expects(mask >= 0);
	if (!sprites.empty()) {
		this->sprites.push_back(SpritePainterEntry(SpritePainterEntryType::SpriteCached, cachedSprites.size(), sprites.size(), mask, layer, tieBreaker, this->sprites.size(), std::move(clip)));
		cachedSprites.reserve(cachedSprites.size() + sprites.size());
		for (auto& s: sprites) {
			cachedSprites.push_back(s.clone(false));
		}
		dirty = true;
	}
}

void SpritePainter::add(const TextRenderer& text, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip)
{
	if (forceCopy) {
		addCopy(text, mask, layer, tieBreaker, clip);
	} else {
		Expects(mask >= 0);
		sprites.push_back(SpritePainterEntry(gsl::span<const TextRenderer>(&text, 1), mask, layer, tieBreaker, sprites.size(), std::move(clip)));
		dirty = true;
	}
}

void SpritePainter::add(TextRenderer&& text, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip)
{
	Expects(mask >= 0);
	sprites.push_back(SpritePainterEntry(SpritePainterEntryType::TextCached, cachedText.size(), 1, mask, layer, tieBreaker, sprites.size(), std::move(clip)));
	cachedText.push_back(text);
	dirty = true;
}

void SpritePainter::addCopy(const TextRenderer& text, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip)
{
	Expects(mask >= 0);
	sprites.push_back(SpritePainterEntry(SpritePainterEntryType::TextCached, cachedText.size(), 1, mask, layer, tieBreaker, sprites.size(), std::move(clip)));
	cachedText.push_back(text);
	dirty = true;
}

void SpritePainter::add(SpritePainterEntry::Callback callback, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip)
{
	Expects(mask >= 0);
	sprites.push_back(SpritePainterEntry(SpritePainterEntryType::Callback, callbacks.size(), 1, mask, layer, tieBreaker, sprites.size(), std::move(clip)));
	callbacks.push_back(std::move(callback));
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
	const auto& cam = painter.getCurrentCamera();
	Rect4f view = cam.getClippingRectangle();

	// Draw!
	for (auto& s : sprites) {
		if ((s.getMask() & mask) != 0) {
			const auto type = s.getType();
			
			if (type == SpritePainterEntryType::SpriteRef) {
				draw(s.getSprites(), painter, view, s.getClip());
			} else if (type == SpritePainterEntryType::SpriteCached) {
				draw(gsl::span<const Sprite>(cachedSprites.data() + s.getIndex(), s.getCount()), painter, view, s.getClip());
			} else if (type == SpritePainterEntryType::TextRef) {
				draw(s.getTexts(), painter, view, s.getClip());
			} else if (type == SpritePainterEntryType::TextCached) {
				draw(gsl::span<const TextRenderer>(cachedText.data() + s.getIndex(), s.getCount()), painter, view, s.getClip());
			} else if (type == SpritePainterEntryType::Callback) {
				draw(callbacks.at(s.getIndex()), painter, s.getClip());
			}
		}
	}
	painter.flush();
}

std::optional<Rect4f> SpritePainter::getBounds() const
{
	std::optional<Rect4f> result;
	
	auto merge = [&](Rect4f aabb, std::optional<Rect4f> clip)
	{
		if (clip) {
			if (!aabb.overlaps(*clip)) {
				return;
			}
			aabb = aabb.intersection(*clip);
		}
		result = result ? result->merge(aabb) : aabb;
	};

	for (auto& s: sprites) {
		const auto type = s.getType();
		const auto clip = s.getClip();

		if (type == SpritePainterEntryType::SpriteRef) {
			for (const auto& sprite: s.getSprites()) {
				merge(sprite.getAABB(), clip);
			}
		} else if (type == SpritePainterEntryType::SpriteCached) {
			for (const auto& sprite: gsl::span<const Sprite>(cachedSprites.data() + s.getIndex(), s.getCount())) {
				merge(sprite.getAABB(), clip);
			}
		} else if (type == SpritePainterEntryType::TextRef) {
			for (const auto& text: s.getTexts()) {
				merge(text.getAABB(), clip);
			}
		} else if (type == SpritePainterEntryType::TextCached) {
			for (const auto& text: gsl::span<const TextRenderer>(cachedText.data() + s.getIndex(), s.getCount())) {
				merge(text.getAABB(), clip);
			}
		} else if (type == SpritePainterEntryType::Callback) {
			// Not included
		}
	}

	return result;
}

SpritePainterMaterialParamUpdater& SpritePainter::getParamUpdater()
{
	return paramUpdater;
}

void SpritePainter::draw(gsl::span<const Sprite> sprites, Painter& painter, Rect4f view, const std::optional<Rect4f>& clip) const
{
	for (const auto& sprite: sprites) {
		if (sprite.isInView(view)) {
			if (paramUpdater.needsToPreProcessessMaterial(sprite)) {
				auto s2 = sprite;
				paramUpdater.preProcessMaterial(s2);
				s2.draw(painter, clip);
			} else {
				sprite.draw(painter, clip);
			}
		}
	}
}

void SpritePainter::draw(gsl::span<const TextRenderer> texts, Painter& painter, Rect4f view, const std::optional<Rect4f>& clip) const
{
	for (const auto& text: texts) {
		text.draw(painter, clip);
	}
}

void SpritePainter::draw(const SpritePainterEntry::Callback& callback, Painter& painter, const std::optional<Rect4f>& clip) const
{
	if (clip) {
		painter.setRelativeClip(clip.value());
	}
	callback(painter);
	if (clip) {
		painter.setClip();
	}
}
