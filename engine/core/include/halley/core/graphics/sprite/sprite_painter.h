#pragma once

#include <halley/data_structures/vector.h>
#include <cstddef>
#include "halley/maths/rect.h"
#include <limits>

namespace Halley
{
	class TextRenderer;
	class String;
	class Sprite;
	class Painter;

	enum class SpritePainterEntryType
	{
		SpriteRef,
		SpriteCached,
		TextRef,
		TextCached
	};

	class SpritePainterEntry
	{
	public:
		SpritePainterEntry(const Sprite& sprite, int mask, int layer, float tieBreaker);
		SpritePainterEntry(const TextRenderer& text, int mask, int layer, float tieBreaker);
		SpritePainterEntry(SpritePainterEntryType type, size_t spriteIdx, int mask, int layer, float tieBreaker);

		bool operator<(const SpritePainterEntry& o) const;
		SpritePainterEntryType getType() const;
		const Sprite& getSprite() const;
		const TextRenderer& getText() const;
		size_t getIndex() const;
		int getMask() const;

	private:
		const void* ptr = nullptr;
		unsigned int index = std::numeric_limits<unsigned int>::max();
		SpritePainterEntryType type;
		int layer;
		int mask;
		float tieBreaker;
	};

	class SpritePainter
	{
	public:
		void start(size_t nSprites);
		void add(const Sprite& sprite, int mask, int layer, float tieBreaker);
		void addCopy(const Sprite& sprite, int mask, int layer, float tieBreaker);
		void add(const TextRenderer& sprite, int mask, int layer, float tieBreaker);
		void addCopy(const TextRenderer& text, int mask, int layer, float tieBreaker);
		void draw(int mask, Painter& painter);

	private:
		Vector<SpritePainterEntry> sprites;
		Vector<Sprite> cachedSprites;
		Vector<TextRenderer> cachedText;
		bool dirty = false;

		void draw(const Sprite& sprite, Painter& painter, Rect4f view);
		void draw(const TextRenderer& text, Painter& painter, Rect4f view);
	};
}
