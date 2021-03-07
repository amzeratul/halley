#pragma once

#include "halley/ui/widgets/ui_clickable.h"
#include "halley/ui/widgets/ui_scroll_pane.h"

namespace Halley {
    class ScrollBackground : public UIClickable {
    public:
		using ZoomListener = std::function<void(float)>;
    	using MousePosListener = std::function<void(Vector2f)>;

        ScrollBackground(String id, UIStyle style, UISizer sizer);
		float getZoomLevel() const;
		void setZoomListener(ZoomListener listener);
    	void setMousePosListener(MousePosListener listener);

    	void setZoomEnabled(bool enabled);

	protected:
		void doSetState(State state) override;
		void update(Time t, bool moved) override;
        void draw(UIPainter& painter) const override;

        UIScrollPane* getScrollPane() const;

        void pressMouse(Vector2f mousePos, int button) override;
		void releaseMouse(Vector2f mousePos, int button) override;
        void onMouseOver(Vector2f mousePos) override;
		void onDoubleClicked(Vector2f mousePos) override;
    	
    private:
		Sprite bg;
		Vector2f mouseStartPos;
		Vector2f startScrollPos;
		Vector2f lastMousePos;
		UIScrollPane* pane = nullptr;
		int zoomExp = 0;
		bool dirty = false;
		bool dragging = false;
    	bool zoomEnabled = true;

		ZoomListener zoomListener;
    	MousePosListener mousePosListener;

        void setDragPos(Vector2f pos);
		void onMouseWheel(const UIEvent& event);
    };
}
