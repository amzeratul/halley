#pragma once

namespace Halley {
    class ScrollBackground : public UIWidget {
    public:
        ScrollBackground(String id, Resources& res, UISizer sizer);

    protected:
        void update(Time t, bool moved) override;
        void draw(UIPainter& painter) const override;

		bool canInteractWithMouse() const override;
		bool isFocusLocked() const override;

		void pressMouse(Vector2f mousePos, int button) override;
		void releaseMouse(Vector2f mousePos, int button) override;
        void onMouseOver(Vector2f mousePos) override;

    private:
		Sprite bg;
		bool dragging = false;
		Vector2f mouseStartPos;
		Vector2f startScrollPos;
		UIScrollPane* pane = nullptr;

        void setDragPos(Vector2f pos);
    };
}
