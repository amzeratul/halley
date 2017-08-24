#pragma once

#include "ui_scroll_pane.h"

namespace Halley {
	class UIStyle;

    class UIScrollBar : public UIWidget {
    public:
		UIScrollBar(UIScrollDirection direction, UIStyle style);
		void setScrollPane(UIScrollPane& pane);

	    bool canInteractWithMouse() const override;
	    void pressMouse(Vector2f mousePos, int button) override;
	    void releaseMouse(Vector2f mousePos, int button) override;

    protected:
		void checkActive() override;
		void update(Time t, bool moved) override;

    private:
		UIScrollDirection direction;
		UIScrollPane* pane = nullptr;

		std::shared_ptr<UIWidget> bar;
		std::shared_ptr<UIWidget> thumb;
    };
}
