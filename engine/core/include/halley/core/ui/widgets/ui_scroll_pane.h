#pragma once

#include "../ui_widget.h"

namespace Halley {
    class UIScrollPane : public UIWidget {
    public:
		UIScrollPane(Vector2f clipSize, bool scrollHorizontal = false, bool scrollVertical = true, Vector2f minSize = {});
		void scrollTo(Vector2f position);

    protected:
	    void update(Time t, bool moved) override;
	    void drawChildren(UIPainter& painter) const override;
	    Vector2f getLayoutMinimumSize() const override;
	    Vector2f getLayoutOriginPosition() const override;

   	private:
		Vector2f clipSize;
		Vector2f scrollPos;
		bool scrollHorizontal;
		bool scrollVertical;
    };
}
