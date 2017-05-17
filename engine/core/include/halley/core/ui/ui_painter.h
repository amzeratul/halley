#pragma once

namespace Halley {
	class TextRenderer;
	class Sprite;
	class SpritePainter;

	class UIPainter {
	public:
		UIPainter(SpritePainter& painter, int mask, int layer);

		void draw(const Sprite& sprite, int layerOffset = 0);
		void draw(const TextRenderer& sprite, int layerOffset = 0);

	private:
		SpritePainter& painter;
		int mask;
		int layer;
		int n;
	};
}