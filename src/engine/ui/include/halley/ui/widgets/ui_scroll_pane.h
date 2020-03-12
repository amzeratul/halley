#pragma once

#include "../ui_widget.h"

namespace Halley {
	enum class UIScrollDirection {
		Horizontal,
		Vertical
	};

	template <>
	struct EnumNames<UIScrollDirection> {
		constexpr std::array<const char*, 2> operator()() const {
			return {{
				"horizontal",
				"vertical"
			}};
		}
	};

    class UIScrollPane : public UIWidget {
    public:
		UIScrollPane(String id, Vector2f clipSize, UISizer&& sizer, bool scrollHorizontal = false, bool scrollVertical = true);
		UIScrollPane(Vector2f clipSize, UISizer&& sizer, bool scrollHorizontal = false, bool scrollVertical = true);

		Vector2f getScrollPosition() const;
		Vector2f getRelativeScrollPosition() const;
		Vector2f getRelativeScrollEndPosition() const;

		void scrollTo(Vector2f position);
		void scrollBy(Vector2f delta);
		void scrollToShow(Rect4f rect, bool center);
		void setRelativeScroll(float position, UIScrollDirection direction);

		float getScrollSpeed() const;
		void setScrollSpeed(float speed);
	    void update(Time t, bool moved) override;

		bool canScroll(UIScrollDirection direction) const;
		float getCoverageSize(UIScrollDirection direction) const;

		void setScrollWheelEnabled(bool enabled);
		bool isScrollWheelEnabled() const;

		void refresh(bool force = false);

		Maybe<float> getMaxChildWidth() const override;
		bool ignoreClip() const override;

		void onChildrenAdded() override;
		void onChildrenRemoved() override;
    	
    protected:
	    void drawChildren(UIPainter& painter) const override;
	    Vector2f getLayoutMinimumSize(bool force) const override;
	    Vector2f getLayoutOriginPosition() const override;
	    bool canInteractWithMouse() const override;
		void onLayout() override;

   	private:
		Vector2f clipSize;
		Vector2f contentsSize;
		Vector2f scrollPos;
		float scrollSpeed = 0;
		bool scrollHorizontal = false;
		bool scrollVertical = false;
		bool scrollWheelEnabled = true;

		void onMouseWheel(const UIEvent& event);
		Vector2f getBasePosition(const String& widgetId);
    };
}
