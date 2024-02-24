#pragma once

#include "halley/ui/widgets/ui_clickable.h"

namespace Halley {
    class BaseCanvas : public UIClickable {
    public:
		using ZoomListener = std::function<void(float)>;
    	using MousePosListener = std::function<void(Vector2f, KeyMods)>;
    	using ScrollListener = std::function<void(Vector2f)>;

        BaseCanvas(String id, UIStyle style, UISizer sizer, std::shared_ptr<InputKeyboard> keyboard);
		float getZoomLevel() const;
		void setZoomListener(ZoomListener listener);
    	void setMousePosListener(MousePosListener listener);
		void setScrollListener(ScrollListener listener);

    	void setZoomEnabled(bool enabled);
		void setScrollEnabled(bool enabled);
		void setLeftClickScrollEnabled(bool enabled);
		void setLeftClickScrollKey(std::optional<KeyCode> key);
		void setMouseMirror(std::shared_ptr<UIWidget> mouseMirror, bool evenWhenDragging = false);
		
        virtual void setScrollPosition(Vector2f pos) = 0;
		virtual Vector2f getScrollPosition() const = 0;
		virtual Vector2f getBasePosition() const = 0;

		void changeZoom(int amount, std::optional<Vector2f> anchor = std::nullopt);

		std::optional<Vector2f> transformToChildSpace(Vector2f pos) const override;

    protected:
		void doSetState(State state) override;
		void update(Time t, bool moved) override;
        void draw(UIPainter& painter) const override;
		void drawAfterChildren(UIPainter& painter) const override;

        void pressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;
		void releaseMouse(Vector2f mousePos, int button) override;
        void onMouseOver(Vector2f mousePos, KeyMods keyMods) override;
		void onDoubleClicked(Vector2f mousePos, KeyMods keyMods) override;

    	void onNewScrollPosition(Vector2f pos) const;
		virtual float getBackgroundScrollSpeed() const;
		virtual Vector2f getBackgroundOffset(Vector2f size) const;
		virtual void refresh();
    	
    private:
		std::shared_ptr<InputKeyboard> keyboard;

    	Sprite bg;
		Sprite border;
		Vector2f bgSize;

    	Vector2f mouseStartPos;
		Vector2f startScrollPos;
		Vector2f lastMousePos;
		int zoomExp = 0;
    	bool zoomEnabled = true;
		bool scrollEnabled = true;
		bool leftClickScrollEnabled = true;
		std::optional<KeyCode> leftClickScrollKey;

		bool dragging = false;
		std::array<bool, 2> draggingButton;

		ZoomListener zoomListener;
    	MousePosListener mousePosListener;
		ScrollListener scrollListener;
		std::shared_ptr<UIWidget> mouseMirror;
		bool mirrorWhenDragging = false;

		void onMouseWheel(const UIEvent& event);
    };
}
