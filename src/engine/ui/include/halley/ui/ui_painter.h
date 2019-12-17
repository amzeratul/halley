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

		void draw(const Sprite& sprite, bool forceCopy = false);
		void draw(const TextRenderer& text, bool forceCopy = false);

		UIPainter clone();
		UIPainter withAdjustedLayer(int delta);
		UIPainter withClip(Maybe<Rect4f> clip);
		UIPainter withMask(int mask);

		Maybe<Rect4f> getClip() const;

	private:
		SpritePainter& painter;
		Maybe<Rect4f> clip;
		int mask;
		int layer;
		int n;
		UIPainter* parent = nullptr;

		float getCurrentPriority();
	};
}
