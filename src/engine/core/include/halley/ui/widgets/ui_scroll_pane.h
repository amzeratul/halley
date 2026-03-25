#pragma once

#include "../ui_widget.h"

namespace Halley {
	enum class UIScrollDirection {
		Horizontal,
		Vertical
	};

	template <>
	struct EnumNames<UIScrollDirection> {
		constexpr auto operator()() const {
			return std::to_array({
				"horizontal",
				"vertical"
			});
		}
	};

    class UIScrollPane : public UIWidget {
    public:
		UIScrollPane(String id, Vector2f clipSize, UISizer&& sizer, bool scrollHorizontal = false, bool scrollVertical = true, float scrollSpeed = 50.0f, bool alwaysSmooth = false);
		[[deprecated]] UIScrollPane(Vector2f clipSize, UISizer&& sizer, bool scrollHorizontal = false, bool scrollVertical = true);

    	void update(Time t, bool moved) override;

    	Vector2f getScrollPosition() const;
		Vector2f getRelativeScrollPosition() const;
		Vector2f getRelativeScrollEndPosition() const;

		void setClipSize(Vector2f clipSize);

		void scrollTo(Vector2f position);
		void scrollBy(Vector2f delta);
		void scrollToShow(Rect4f rect, bool center, bool continuous);
		void setRelativeScroll(float position, UIScrollDirection direction);

		float getScrollSpeed() const;
		void setScrollSpeed(float speed);
		bool isScrolling(float threshold = 0) const;

		void setMarquee(std::optional<Vector2f> speed);
		std::optional<Vector2f> getMarque() const;

		bool canScroll(UIScrollDirection direction) const;
		float getCoverageSize(UIScrollDirection direction) const;

		void setScrollWheelEnabled(bool enabled);
		bool isScrollWheelEnabled() const;

		void refresh(bool force = false);

		std::optional<float> getMaxChildWidth() const override;
		bool ignoreClip() const override;

		void onChildrenAdded() override;
		void onChildrenRemoved() override;

		bool canChildrenInteractWithMouseAt(Vector2f pos) const override;
    	
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
		std::optional<Vector2f> targetScrollTo;
		std::optional<Vector2f> marquee;
		float scrollSpeed = 0;
		bool alwaysSmooth = false;
		bool scrollHorizontal = false;
		bool scrollVertical = false;
		bool scrollWheelEnabled = true;
		Time lastDeltaT = 0;
		Vector2f marqueePhase;

		void onMouseWheel(const UIEvent& event);
		Vector2f getBasePosition(const String& widgetId);
		Vector2f clampScrollPos(Vector2f pos) const;

		void refreshScrollWheelHandler();
    };
}
