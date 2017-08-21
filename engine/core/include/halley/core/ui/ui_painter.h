#pragma once
#include "halley/maths/rect.h"
#include "halley/data_structures/maybe.h"

namespace Halley {
	class TextRenderer;
	class Sprite;
	class SpritePainter;

	class UIPainter {
	public:
		UIPainter(SpritePainter& painter, int mask, int layer);

		void draw(const Sprite& sprite);
		void draw(const TextRenderer& text);

		UIPainter clone() const;
		UIPainter withAdjustedLayer(int delta) const;
		UIPainter withClip(Rect4f clip) const;

	private:
		SpritePainter& painter;
		Maybe<Rect4f> clip;
		int mask;
		int layer;
		int n;
	};
}
