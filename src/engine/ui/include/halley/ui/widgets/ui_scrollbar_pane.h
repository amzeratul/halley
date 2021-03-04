#pragma once

#include "ui_scrollbar.h"
#include "ui_scroll_pane.h"

namespace Halley {
	class UIScrollBarPane : public UIWidget {
	public:
		UIScrollBarPane(String id, Vector2f clipSize, UIStyle style, UISizer&& sizer, bool scrollHorizontal = false, bool scrollVertical = true, bool alwaysShow = true, Vector2f minSize = {});

		std::shared_ptr<UIScrollPane> getPane() const;

		void add(std::shared_ptr<IUIElement> element, float proportion = 0, Vector4f border = Vector4f(), int fillFlags = UISizerFillFlags::Fill, Vector2f position = Vector2f()) override;
		void addSpacer(float size) override;
		void addStretchSpacer(float proportion = 0) override;

		void setAlwaysShow(bool alwaysShow);

	private:
		std::shared_ptr<UIScrollPane> pane;
		std::shared_ptr<UIScrollBar> hBar;
		std::shared_ptr<UIScrollBar> vBar;
	};
}
