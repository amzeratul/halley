#pragma once

#include "base_canvas.h"

namespace Halley {
    class InfiniCanvas : public BaseCanvas {
    public:
        InfiniCanvas(String id, UIStyle style, UISizer sizer, std::shared_ptr<InputKeyboard> keyboard);

	protected:
    	void setScrollPosition(Vector2f pos) override;
		Vector2f getScrollPosition() const override;
		Vector2f getBasePosition() const override;
		Vector2f getLayoutOriginPosition() const override;
		void drawChildren(UIPainter& painter) const override;

	private:
		Vector2f scrollPos;
    };
}
