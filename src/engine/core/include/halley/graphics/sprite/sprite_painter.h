#pragma once

#include <halley/data_structures/vector.h>
#include <cstddef>
#include "halley/maths/rect.h"
#include <limits>
#include <optional>

#include "halley/data_structures/hash_map.h"
#include "halley/entity/services/dev_service.h"
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
		gsl::span<const Sprite> getSprites(const Vector<Sprite>& cached) const;
		gsl::span<const TextRenderer> getTexts(const Vector<TextRenderer>& cached) const;
		uint32_t getIndex() const;
		uint32_t getCount() const;
		int getMask() const;
		const std::optional<Rect4f>& getClip() const;

		Rect4f getBounds(const Rect4f& view, const Vector<Sprite>& cachedSprites, const Vector<TextRenderer>& cachedText) const;
		bool isCompatibleWith(const SpritePainterEntry& other, const Vector<Sprite>& cachedSprites, const Vector<TextRenderer>& cachedText) const;

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
		using Callback = std::function<void(MaterialUpdater&, std::string_view, std::string_view)>;

		SpritePainterMaterialParamUpdater();

		void update(Time t);
		void copyPrevious(const SpritePainterMaterialParamUpdater& previous);
		void setHandle(String id, Callback callback);

		bool needsToPreProcessessMaterial(const Sprite& sprite) const;
		void preProcessMaterial(Sprite& sprite) const;

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
		void clear();
		
		void add(const Sprite& sprite, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip = {});
		void add(Sprite&& sprite, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip = {});
		void addCopy(const Sprite& sprite, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip = {});
		void add(gsl::span<const Sprite> sprites, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip = {});
		void addCopy(gsl::span<const Sprite> sprites, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip = {});
		void add(const TextRenderer& sprite, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip = {});
		void add(TextRenderer&& text, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip = {});
		void addCopy(const TextRenderer& text, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip = {});
		void add(SpritePainterEntry::Callback callback, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip = {});
		void add(Rect4f bounds);

		void draw(int mask, Painter& painter);
		std::optional<Rect4f> getBounds() const;

		SpritePainterMaterialParamUpdater& getParamUpdater();

		void setWaitForSpriteLoad(bool wait);

	private:
		Vector<SpritePainterEntry> sprites;
		Vector<Sprite> cachedSprites;
		Vector<TextRenderer> cachedText;
		Vector<SpritePainterEntry::Callback> callbacks;
		Vector<Rect4f> extraBounds;
		bool dirty = false;
		bool forceCopy = false;
		bool waitForSpriteLoad = true;
		SpritePainterMaterialParamUpdater paramUpdater;

		void draw(gsl::span<const Sprite> sprite, Painter& painter, Rect4f view, const std::optional<Rect4f>& clip) const;
		void draw(gsl::span<const TextRenderer> text, Painter& painter, Rect4f view, const std::optional<Rect4f>& clip) const;
		void draw(const SpritePainterEntry::Callback& callback, Painter& painter, const std::optional<Rect4f>& clip) const;

		Vector<uint32_t> getSpriteDrawOrder(int mask, Rect4f view) const;
		Vector<uint32_t> getSpriteDrawOrderReordered(int mask, Rect4f view) const;
	};
}
