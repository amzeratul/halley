#pragma once

#include "halley/ui/widgets/ui_clickable.h"

namespace Halley {
    class BaseCanvas : public UIClickable {
    public:
		using ZoomListener = std::function<void(float)>;
    	using MousePosListener = std::function<void(Vector2f)>;

        BaseCanvas(String id, UIStyle style, UISizer sizer, std::shared_ptr<InputKeyboard> keyboard);
		float getZoomLevel() const;
		void setZoomListener(ZoomListener listener);
    	void setMousePosListener(MousePosListener listener);

    	void setZoomEnabled(bool enabled);
		void setScrollEnabled(bool enabled);
		void setMouseMirror(std::shared_ptr<UIWidget> mouseMirror);
		
	protected:
		void doSetState(State state) override;
		void update(Time t, bool moved) override;
        void draw(UIPainter& painter) const override;

        void pressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;
		void releaseMouse(Vector2f mousePos, int button) override;
        void onMouseOver(Vector2f mousePos) override;
		void onDoubleClicked(Vector2f mousePos, KeyMods keyMods) override;

        virtual void setScrollPosition(Vector2f pos) = 0;
		virtual Vector2f getScrollPosition() const = 0;
		virtual Vector2f getBasePosition() const = 0;
		virtual void refresh();
    	
    private:
		std::shared_ptr<InputKeyboard> keyboard;
		Sprite bg;
		Vector2f mouseStartPos;
		Vector2f startScrollPos;
		Vector2f lastMousePos;
		int zoomExp = 0;
		bool dirty = false;
    	bool zoomEnabled = true;
		bool scrollEnabled = true;

		bool dragging = false;
		std::array<bool, 2> draggingButton;

		ZoomListener zoomListener;
    	MousePosListener mousePosListener;
		std::shared_ptr<UIWidget> mouseMirror;

		void onMouseWheel(const UIEvent& event);
    };
}
