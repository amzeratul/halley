#pragma once

#include <vector>
#include <cstddef>

namespace Halley
{
	class Sprite;
	class Painter;

	class SpritePainterEntry
	{
	public:
		bool operator<(const SpritePainterEntry& o) const {
			if (layer != o.layer) {
				return layer < o.layer;
			} else if (tieBreaker != o.tieBreaker) {
				return tieBreaker < o.tieBreaker;
			} else {
				return sprite < o.sprite;
			}
		}

		SpritePainterEntry(Sprite& sprite, int layer, int tieBreaker);
		Sprite& getSprite() const { return *sprite; }

	private:
		Sprite* sprite;
		int layer;
		int tieBreaker;
	};

	class SpritePainter
	{
	public:
		void start(size_t nSprites);
		void add(Sprite& sprite, int layer, int tieBreaker);
		void draw(Painter& painter);

	private:
		std::vector<SpritePainterEntry> sprites;
	};
}
