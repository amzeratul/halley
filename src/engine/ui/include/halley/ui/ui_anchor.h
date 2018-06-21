#pragma once

#include <halley/maths/vector2.h>
#include "halley/maths/rect.h"
#include "halley/data_structures/maybe.h"

namespace Halley {
	class UIParent;
	class UIWidget;

	class UIAnchor
	{
	public:
		UIAnchor(Vector2f relativePos = Vector2f(0.5f, 0.5f), Vector2f relativeAlignment = Vector2f(0.5f, 0.5f), Vector2f absoluteOffset = Vector2f(), Maybe<Rect4f> bounds = {});

		Vector2f getRelativePos() const;
		Vector2f getRelativeAlignment() const;
		Vector2f getAbsoluteOffset() const;
		Maybe<Rect4f> getBounds() const;

		UIAnchor& setBounds(UIParent& parent);
		UIAnchor& setAutoBounds(bool enabled);
		
		void position(UIWidget& widget) const;

		UIAnchor operator*(float f) const;
		UIAnchor operator+(const UIAnchor& other) const;

	private:
		Vector2f relativePos;
		Vector2f relativeAlignment;
		Vector2f absoluteOffset;
		Maybe<Rect4f> bounds;
		bool autoBounds = false;
	};
}
