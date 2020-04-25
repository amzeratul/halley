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
		UIPainter(UIPainter&& other) noexcept = default;

		UIPainter& operator=(UIPainter&& other) = default;

		void draw(const Sprite& sprite, bool forceCopy = false);
		void draw(const TextRenderer& text, bool forceCopy = false);

		UIPainter clone() const;
		UIPainter withAdjustedLayer(int delta) const;
		UIPainter withClip(std::optional<Rect4f> clip) const;
		UIPainter withMask(int mask) const;
		UIPainter withNoClip() const;

		std::optional<Rect4f> getClip() const;

	private:
		SpritePainter* painter = nullptr;
		std::optional<Rect4f> clip;
		int mask;
		int layer;
		mutable int currentPriority = 0;
		const UIPainter* rootPainter = nullptr;

		float getCurrentPriorityAndIncrement() const;
	};
}
