#pragma once

#include "../ui_widget.h"

namespace Halley {
	class UIStyle;
	class UIScrollPane;

    class UIScrollBar : public UIWidget {
    public:
		enum class Type {
			Horizontal,
			Vertical
		};

		UIScrollBar(Type type, std::shared_ptr<UIStyle> style);
		void setScrollPane(UIScrollPane& pane);

    private:
		Type type;
		UIScrollPane* pane = nullptr;
    };
}