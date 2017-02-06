#pragma once

#include "halley/maths/vector2.h"
#include "halley/maths/rect.h"

namespace Halley {
	class IUIElement {
	public:
		virtual ~IUIElement() {}

		virtual Vector2f computeMinimumSize() const = 0;
		virtual void setRect(Rect4f rect) = 0;
		virtual bool isEnabled() const = 0;
	};
	using UIElementPtr = std::shared_ptr<IUIElement>;
}
