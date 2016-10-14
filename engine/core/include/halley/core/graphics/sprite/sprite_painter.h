#pragma once

#include <halley/data_structures/vector.h>
#include <cstddef>

namespace Halley
{
	class Sprite;
	class Painter;

	class SpritePainterEntry
	{
	public:
		SpritePainterEntry(Sprite& sprite, int layer, float tieBreaker);
		SpritePainterEntry(size_t spriteIdx, int layer, float tieBreaker);

		bool operator<(const SpritePainterEntry& o) const;
		bool hasSprite() const;
		Sprite& getSprite() const;
		size_t getIndex() const;

	private:
		Sprite* sprite = nullptr;
		size_t index = -1;
		int layer;
		float tieBreaker;
	};

	class SpritePainter
	{
	public:
		void start(size_t nSprites);
		void add(Sprite& sprite, int layer, float tieBreaker);
		void addCopy(const Sprite& sprite, int layer, float tieBreaker);
		void draw(Painter& painter);

	private:
		Vector<SpritePainterEntry> sprites;
		Vector<Sprite> cached;
	};
}
