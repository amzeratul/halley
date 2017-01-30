#pragma once

#include "../ui_widget.h"
#include "halley/core/graphics/sprite/sprite.h"
#include "halley/core/graphics/text/text_renderer.h"

namespace Halley {
	class UIStyle;
	class UIValidator;

	class UIDropdown : public UIWidget {
	public:
		explicit UIDropdown(String id, std::shared_ptr<UIStyle> style, const std::vector<String>& options, int defaultOption = 0);
		bool isFocusable() const override;

	protected:
		void draw(UIPainter& painter) const override;
		void update(Time t, bool moved) override;

	private:
		Sprite sprite;
		TextRenderer label;
		std::shared_ptr<UIStyle> style;

		std::vector<String> options;
		int curOption = 0;

		bool wasHover;
    };
}
