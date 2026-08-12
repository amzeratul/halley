#pragma once
#include "halley/ui/ui_widget.h"

namespace Halley {
	class UIMaskOverride : public UIWidget {
	public:
		UIMaskOverride(String id, Vector2f minSize = {}, std::optional<UISizer> sizer = {}, Vector4f innerBorder = {});

		void setMaskOverride(std::optional<int> mask);
		void drawChildren(UIPainter& painter) const override;

	private:
		std::optional<int> mask;
	};
}
