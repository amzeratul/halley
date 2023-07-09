#pragma once

#include "base_canvas.h"
#include "halley/ui/widgets/ui_scroll_pane.h"

namespace Halley {
    class ScrollBackground : public BaseCanvas {
    public:
        ScrollBackground(String id, UIStyle style, UISizer sizer, std::shared_ptr<InputKeyboard> keyboard);

	protected:
        UIScrollPane* getScrollPane() const;

    	void setScrollPosition(Vector2f pos) override;
		Vector2f getScrollPosition() const override;
		Vector2f getBasePosition() const override;
        void refresh() override;
        float getBackgroundScrollSpeed() const override;
        Vector2f getBackgroundOffset(Vector2f size) const override;

	private:
		mutable UIScrollPane* pane = nullptr;
    };
}
