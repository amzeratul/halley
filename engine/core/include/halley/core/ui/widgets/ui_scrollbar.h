#pragma once

#include "ui_scroll_pane.h"

namespace Halley {
	class UIStyle;

    class UIScrollBar : public UIWidget {
    public:
		UIScrollBar(UIScrollDirection direction, std::shared_ptr<UIStyle> style);
		void setScrollPane(UIScrollPane& pane);

    protected:
		void checkActive() override;
		void update(Time t, bool moved) override;

    private:
		UIScrollDirection direction;
		UIScrollPane* pane = nullptr;

		std::shared_ptr<UIWidget> bar;
		std::shared_ptr<UIWidget> thumb;

		bool isNeeded() const;
    };
}
