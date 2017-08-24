#pragma once

#include "ui_scrollbar.h"
#include "ui_scroll_pane.h"

namespace Halley {
	class UIScrollBarPane : public UIWidget {
	public:
		UIScrollBarPane(Vector2f clipSize, std::shared_ptr<UIStyle> style, bool scrollHorizontal = false, bool scrollVertical = true, Vector2f minSize = {});

		void add(std::shared_ptr<UIWidget> widget, float proportion = 0, Vector4f border = Vector4f(), int fillFlags = UISizerFillFlags::Fill) override;
		void add(std::shared_ptr<UISizer> sizer, float proportion = 0, Vector4f border = Vector4f(), int fillFlags = UISizerFillFlags::Fill) override;
		void addSpacer(float size) override;
		void addStretchSpacer(float proportion = 0) override;

	private:
		std::shared_ptr<UIScrollPane> pane;
		std::shared_ptr<UIScrollBar> hBar;
		std::shared_ptr<UIScrollBar> vBar;
	};
}
