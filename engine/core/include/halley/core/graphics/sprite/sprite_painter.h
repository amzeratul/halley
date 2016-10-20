#pragma once

#include <halley/data_structures/vector.h>
#include <cstddef>

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
		SpritePainterEntry(Sprite& sprite, int layer, float tieBreaker);
		SpritePainterEntry(TextRenderer& text, int layer, float tieBreaker);
		SpritePainterEntry(SpritePainterEntryType type, size_t spriteIdx, int layer, float tieBreaker);

		bool operator<(const SpritePainterEntry& o) const;
		SpritePainterEntryType getType() const;
		Sprite& getSprite() const;
		TextRenderer& getText() const;
		size_t getIndex() const;

	private:
		void* ptr = nullptr;
		unsigned int index = -1;
		SpritePainterEntryType type;
		int layer;
		float tieBreaker;
	};

	class SpritePainter
	{
	public:
		void start(size_t nSprites);
		void add(Sprite& sprite, int layer, float tieBreaker);
		void addCopy(const Sprite& sprite, int layer, float tieBreaker);
		void add(TextRenderer& sprite, int layer, float tieBreaker);
		void addCopy(const TextRenderer& text, int layer, float tieBreaker);
		void draw(Painter& painter);

	private:
		Vector<SpritePainterEntry> sprites;
		Vector<Sprite> cachedSprites;
		Vector<TextRenderer> cachedText;
	};
}
