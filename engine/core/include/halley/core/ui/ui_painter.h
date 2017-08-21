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
		void draw(const TextRenderer& sprite);

		UIPainter clone() const;
		UIPainter withAdjustedLayer(int delta) const;
		UIPainter withClip(Rect4i clip) const;

	private:
		SpritePainter& painter;
		Maybe<Rect4i> clip;
		int mask;
		int layer;
		int n;
	};
}
