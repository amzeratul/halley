#pragma once

#include "../ui_widget.h"

namespace Halley {
	enum class UIScrollDirection {
		Horizontal,
		Vertical
	};

    class UIScrollPane : public UIWidget {
    public:
		UIScrollPane(Vector2f clipSize, bool scrollHorizontal = false, bool scrollVertical = true, Vector2f minSize = {});

		Vector2f getScrollPosition() const;
		void scrollTo(Vector2f position);
		void scrollBy(Vector2f delta);
		void scrollToShow(Rect4f rect, bool center);

		float getScrollSpeed() const;
		void setScrollSpeed(float speed);
	    void update(Time t, bool moved) override;

		bool canScroll(UIScrollDirection direction) const;
		float getCoverageSize(UIScrollDirection direction) const;

    protected:
	    void drawChildren(UIPainter& painter) const override;
	    Vector2f getLayoutMinimumSize() const override;
	    Vector2f getLayoutOriginPosition() const override;
	    bool canInteractWithMouse() const override;

   	private:
		Vector2f clipSize;
		Vector2f contentsSize;
		Vector2f scrollPos;
		float scrollSpeed;
		bool scrollHorizontal;
		bool scrollVertical;

		void onMouseWheel(const UIEvent& event);
    };
}
