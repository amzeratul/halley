#pragma once

#include <halley/data_structures/vector.h>
#include <cstddef>
#include "halley/maths/rect.h"
#include <limits>
#include <optional>

#include "halley/data_structures/hash_map.h"
#include "halley/time/halleytime.h"

namespace Halley
{
	class TextRenderer;
	class String;
	class Sprite;
	class Painter;
	class Material;

	enum class SpritePainterEntryType
	{
		SpriteRef,
		SpriteCached,
		TextRef,
		TextCached,
		Callback
	};

	class SpritePainterEntry
	{
	public:
		using Callback = std::function<void(Painter&)>;
		
		SpritePainterEntry(gsl::span<const Sprite> sprites, int mask, int layer, float tieBreaker, size_t insertOrder, std::optional<Rect4f> clip);
		SpritePainterEntry(gsl::span<const TextRenderer> texts, int mask, int layer, float tieBreaker, size_t insertOrder, std::optional<Rect4f> clip);
		SpritePainterEntry(SpritePainterEntryType type, size_t spriteIdx, size_t count, int mask, int layer, float tieBreaker, size_t insertOrder, std::optional<Rect4f> clip);

		bool operator<(const SpritePainterEntry& o) const;
		SpritePainterEntryType getType() const;
		gsl::span<const Sprite> getSprites() const;
		gsl::span<const TextRenderer> getTexts() const;
		uint32_t getIndex() const;
		uint32_t getCount() const;
		int getMask() const;
		const std::optional<Rect4f>& getClip() const;

	private:
		const void* ptr = nullptr;
		uint32_t count = 0;
		uint32_t index = std::numeric_limits<uint32_t>::max();
		SpritePainterEntryType type;
		int layer;
		int mask;
		float tieBreaker;
		size_t insertOrder;
		std::optional<Rect4f> clip;
	};

	class MaterialUpdater;

	class SpritePainterMaterialParamUpdater {
	public:
		using Callback = std::function<void(MaterialUpdater&, const String&, const String&)>;

		SpritePainterMaterialParamUpdater();

		void update(Time t);
		void copyPrevious(const SpritePainterMaterialParamUpdater& previous);

		void preProcessMaterial(Sprite& sprite);
		void setHandle(String id, Callback callback);

	private:
		Time curTime = 0;
		HashMap<String, Callback> handles;
	};

	class SpritePainter
	{
	public:
		void update(Time t);
		void copyPrevious(const SpritePainter& prev);

		void start(bool forceCopy = false);
		
		void add(const Sprite& sprite, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip = {});
		void add(Sprite&& sprite, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip = {});
		void addCopy(const Sprite& sprite, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip = {});
		void add(gsl::span<const Sprite> sprites, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip = {});
		void addCopy(gsl::span<const Sprite> sprites, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip = {});
		void add(const TextRenderer& sprite, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip = {});
		void add(TextRenderer&& text, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip = {});
		void addCopy(const TextRenderer& text, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip = {});
		void add(SpritePainterEntry::Callback callback, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip = {});
		
		void draw(int mask, Painter& painter);
		std::optional<Rect4f> getBounds() const;

	private:
		Vector<SpritePainterEntry> sprites;
		Vector<Sprite> cachedSprites;
		Vector<TextRenderer> cachedText;
		Vector<SpritePainterEntry::Callback> callbacks;
		bool dirty = false;
		bool forceCopy = false;
		SpritePainterMaterialParamUpdater paramUpdater;

		void draw(gsl::span<const Sprite> sprite, Painter& painter, Rect4f view, const std::optional<Rect4f>& clip) const;
		void draw(gsl::span<const TextRenderer> text, Painter& painter, Rect4f view, const std::optional<Rect4f>& clip) const;
		void draw(const SpritePainterEntry::Callback& callback, Painter& painter, const std::optional<Rect4f>& clip) const;
	};
}
