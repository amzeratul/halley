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

gsl::span<const Sprite> SpritePainterEntry::getSprites(const Vector<Sprite>& cached) const
{
	assert(type == SpritePainterEntryType::SpriteRef || type == SpritePainterEntryType::SpriteCached);

	if (type == SpritePainterEntryType::SpriteRef) {
		assert(ptr != nullptr);
		return { static_cast<const Sprite*>(ptr), count };
	} else {
		return { cached.data() + index, count };
	}
}

gsl::span<const TextRenderer> SpritePainterEntry::getTexts(const Vector<TextRenderer>& cached) const
{
	assert(type == SpritePainterEntryType::TextRef || type == SpritePainterEntryType::TextCached);

	if (type == SpritePainterEntryType::TextRef) {
		assert(ptr != nullptr);
		return { static_cast<const TextRenderer*>(ptr), count };
	} else {
		return { cached.data() + index, count };
	}
}

uint32_t SpritePainterEntry::getIndex() const
{
	assert(ptr == nullptr);
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

Rect4f SpritePainterEntry::getBounds(const Rect4f& view, const Vector<Sprite>& cachedSprites, const Vector<TextRenderer>& cachedText) const
{
	if (type == SpritePainterEntryType::SpriteCached || type == SpritePainterEntryType::SpriteRef) {
		bool first = true;
		Rect4f result;
		for (const auto& s: getSprites(cachedSprites)) {
			auto r = s.getAABB();
			if (first) {
				result = r;
				first = false;
			} else {
				result = result.merge(r);
			}
		}
		return result;
	} else if (type == SpritePainterEntryType::TextCached || type == SpritePainterEntryType::TextRef) {
		bool first = true;
		Rect4f result;
		for (const auto& s: getTexts(cachedText)) {
			auto r = s.getAABB();
			if (first) {
				result = r;
				first = false;
			} else {
				result = result.merge(r);
			}
		}
		return result;
	} else {
		return view;
	}
}

bool SpritePainterEntry::isCompatibleWith(const SpritePainterEntry& other, const Vector<Sprite>& cachedSprites, const Vector<TextRenderer>& cachedText) const
{
	if (type != other.type || clip != other.clip) {
		return false;
	}

	if (type == SpritePainterEntryType::SpriteCached || type == SpritePainterEntryType::SpriteRef) {
		const auto& s0 = getSprites(cachedSprites)[0];
		const auto& s1 = other.getSprites(cachedSprites)[0];

		// Treat no material as compatible, since it'll be ignored by the renderer a little after this anyway
		if (!s0.hasMaterial() || !s1.hasMaterial()) {
			return true;
		}

		return s0.getMaterial().isCompatibleWith(s1.getMaterial());
	} else if (type == SpritePainterEntryType::TextCached || type == SpritePainterEntryType::TextRef) {
		return getTexts(cachedText)[0].isCompatibleWith(other.getTexts(cachedText)[0]);
	} else {
		return false;
	}
}

SpritePainterMaterialParamUpdater::SpritePainterMaterialParamUpdater()
{
	setHandle("halley.texSize", [this] (MaterialUpdater& material, std::string_view uniformName, std::string_view autoVarArgs)
	{
		if (const auto idx = stringViewToInt(autoVarArgs)) {
			const auto tex = material.getTexture(*idx);
			material.set(uniformName, tex ? Vector2f(tex->getSize()) : Vector2f());
		}
	});

	setHandle("halley.texBPP", [this] (MaterialUpdater& material, std::string_view uniformName, std::string_view autoVarArgs)
	{
		if (const auto idx = stringViewToInt(autoVarArgs)) {
			const auto tex = material.getTexture(*idx);
			material.set(uniformName, tex ? TextureDescriptor::getBytesPerPixel(tex->getDescriptor().format) : 1);
		}
	});

	setHandle("halley.timeLoop", [this] (MaterialUpdater& material, std::string_view uniformName, std::string_view autoVarArgs)
	{
		if (const auto cycleLen = stringViewToDouble(autoVarArgs)) {
			const auto value = std::fmod(curTime / *cycleLen, 1.0);
			material.set(uniformName, static_cast<float>(value));
		}
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
			const auto& var = std::string_view(uniform.autoVariable);
			if (!var.empty()) {
				const auto colonPos = var.find(':');
				std::string_view split0 = var.substr(0, colonPos);
				std::string_view split1;
				if (colonPos != std::string_view::npos) {
					split1 = var.substr(colonPos + 1);
				}

				const auto iter = handles.find(split0);
				if (iter != handles.end()) {
					if (!mat) {
						mat = sprite.getMutableMaterial();
					}
					iter->second(*mat, uniform.name, split1);
				}
			}
		}
	}
}

bool SpritePainterMaterialParamUpdater::needsToPreProcessessMaterial(const Sprite& sprite) const
{
	return sprite.hasMaterial() && sprite.getMaterial().getDefinition().hasAutoVariables();
}

SpritePainter::SpritePainter()
	: memoryPool(256 * 1024)
{
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
	clear();
}

void SpritePainter::clear()
{
	sprites.clear();
	cachedSprites.clear();
	cachedText.clear();
	memoryPool.reset();
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

void SpritePainter::add(Rect4f bounds)
{
	extraBounds.push_back(bounds);
}

void SpritePainter::draw(int mask, Painter& painter)
{
	if (dirty) {
		std::sort(sprites.begin(), sprites.end());
		dirty = false;
	}

	// View
	const Rect4f view = painter.getCurrentCamera().getClippingRectangle();

	// Draw!
	for (auto spriteIdx: getSpriteDrawOrder(mask, view, true)) {
		auto& s = sprites[spriteIdx];
		const auto type = s.getType();
		
		if (type == SpritePainterEntryType::SpriteRef || type == SpritePainterEntryType::SpriteCached) {
			draw(s.getSprites(cachedSprites), painter, view, s.getClip());
		} else if (type == SpritePainterEntryType::TextRef || type == SpritePainterEntryType::TextCached) {
			draw(s.getTexts(cachedText), painter, view, s.getClip());
		} else if (type == SpritePainterEntryType::Callback) {
			draw(callbacks.at(s.getIndex()), painter, s.getClip());
		}
	}
	painter.flush();
}

Vector<uint32_t> SpritePainter::getSpriteDrawOrder(int mask, Rect4f view, bool reorder) const
{
	if (reorder) {
		return getSpriteDrawOrderReordered(mask, view);
	}

	Vector<uint32_t> result;
	const auto nTotal = static_cast<uint32_t>(sprites.size());
	for (uint32_t i = 0; i < nTotal; ++i) {
		auto& s = sprites[i];

		if ((s.getMask() & mask) != 0) {
			result.emplace_back(i);
		}
	}
	return result;
}

Vector<uint32_t> SpritePainter::getSpriteDrawOrderReordered(int mask, Rect4f view) const
{
	struct Entry {
		uint32_t idx = 0;
		bool assigned = false;
		Rect4f bounds;

		Entry(uint32_t idx = 0, Rect4f bounds = {})
			: idx(idx)
			, bounds(bounds)
		{}
	};

	auto entries = Vector<Entry, TempPoolAllocator<Entry>>(TempPoolAllocator<Entry>(memoryPool));
	auto skipped = Vector<Rect4f, TempPoolAllocator<Rect4f>>(TempPoolAllocator<Entry>(memoryPool));
	skipped.reserve(64);
	constexpr int maxSkipsInARow = 16;

	// Generate filtered sprite draw order, and sprite bounds
	const auto nTotal = static_cast<uint32_t>(sprites.size());
	for (uint32_t i = 0; i < nTotal; ++i) {
		auto& s = sprites[i];

		if ((s.getMask() & mask) != 0) {
			entries.emplace_back(i, s.getBounds(view, cachedSprites, cachedText));
		}
	}
	const auto n = static_cast<uint32_t>(entries.size());

	Vector<uint32_t> result;
	result.reserve(entries.size());

	auto overlapsAny = [](const Rect4f& a, const Rect4f bCombined, const Vector<Rect4f, TempPoolAllocator<Rect4f>>& bs)
	{
		if (bs.empty() || !a.overlaps(bCombined)) {
			return false;
		}

		for (const auto& b: bs) {
			if (a.overlaps(b)) {
				return true;
			}
		}
		return false;
	};

	// Go through everyone, adding to final list, including any re-ordering
	for (uint32_t i = 0; i < n; ++i) {
		auto& entry = entries[i];
		if (entry.assigned) {
			continue;
		}

		// Add to result
		result.push_back(entry.idx);
		entry.assigned = true;

		if (sprites[entry.idx].getType() == SpritePainterEntryType::Callback) {
			continue;
		}

		// Look ahead and see if anyone else can join
		skipped.clear();
		Rect4f combinedSkipped;
		int skipsInARow = 0;
		for (uint32_t j = i + 1; j < n; ++j) {
			auto& other = entries[j];
			if (other.assigned) {
				continue;
			}

			if (sprites[other.idx].getType() == SpritePainterEntryType::Callback) {
				break;
			}

			if (sprites[entry.idx].isCompatibleWith(sprites[other.idx], cachedSprites, cachedText) && !overlapsAny(other.bounds, combinedSkipped, skipped)) {
				result.push_back(other.idx);
				other.assigned = true;
				skipsInARow = 0;
			} else {
				combinedSkipped = skipped.empty() ? other.bounds : combinedSkipped.merge(other.bounds);
				skipped.push_back(other.bounds);
				++skipsInARow;

				if (skipsInARow >= maxSkipsInARow) {
					break;
				}
			}
		}
	}

	return result;
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

		if (type == SpritePainterEntryType::SpriteRef || type == SpritePainterEntryType::SpriteCached) {
			for (const auto& sprite: s.getSprites(cachedSprites)) {
				merge(sprite.getAABB(), clip);
			}
		} else if (type == SpritePainterEntryType::TextRef || type == SpritePainterEntryType::TextCached) {
			for (const auto& text: s.getTexts(cachedText)) {
				merge(text.getAABB(), clip);
			}
		} else if (type == SpritePainterEntryType::Callback) {
			// Not included
		}
	}

	for (auto& b: extraBounds) {
		merge(b, {});
	}

	return result;
}

SpritePainterMaterialParamUpdater& SpritePainter::getParamUpdater()
{
	return paramUpdater;
}

void SpritePainter::setWaitForSpriteLoad(bool wait)
{
	waitForSpriteLoad = wait;
}

void SpritePainter::draw(gsl::span<const Sprite> sprites, Painter& painter, Rect4f view, const std::optional<Rect4f>& clip) const
{
	for (const auto& sprite: sprites) {
		if (sprite.isInView(view)) {
			// The logic is a bit confusing here - if we're waiting, just go ahead, as the code will eventually wait
			// If we're not waiting, skip this sprite if it's not loaded
			if (waitForSpriteLoad || sprite.isLoaded()) {
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
