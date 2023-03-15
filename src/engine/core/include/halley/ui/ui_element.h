#pragma once

#include "halley/maths/vector2.h"
#include "halley/maths/rect.h"

namespace Halley {
	class UISizer;

	class IUIElement {
	public:
		class IUIElementListener {
		public:
			virtual ~IUIElementListener() = default;
			virtual void onPlaceInside(Rect4f rect, Rect4f origRect, const std::shared_ptr<IUIElement>& element, UISizer& sizer) = 0;
		};

		virtual ~IUIElement() {}

		virtual Vector2f getLayoutMinimumSize(bool force) const = 0;
		virtual void setRect(Rect4f rect, IUIElementListener* listener) = 0;
		virtual bool isActive() const = 0;
	};
	using UIElementPtr = std::shared_ptr<IUIElement>;
}
