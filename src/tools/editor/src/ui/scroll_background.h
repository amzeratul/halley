#pragma once

namespace Halley {
    class ScrollBackground : public UIWidget {
    public:
		using ZoomListener = std::function<void(float)>;

        ScrollBackground(String id, Resources& res, UISizer sizer);
		float getZoomLevel() const;
		void setZoomListener(ZoomListener listener);

    protected:
        void update(Time t, bool moved) override;
        void draw(UIPainter& painter) const override;

		bool canInteractWithMouse() const override;
		bool isFocusLocked() const override;
        UIScrollPane* getScrollPane() const;

        void pressMouse(Vector2f mousePos, int button) override;
		void releaseMouse(Vector2f mousePos, int button) override;
        void onMouseOver(Vector2f mousePos) override;

    private:
		Sprite bg;
		Vector2f mouseStartPos;
		Vector2f startScrollPos;
		Vector2f lastMousePos;
		UIScrollPane* pane = nullptr;
		int zoomExp = 0;
		bool dirty = false;
		bool dragging = false;

		ZoomListener zoomListener;

        void setDragPos(Vector2f pos);
		void onMouseWheel(const UIEvent& event);
    };
}
