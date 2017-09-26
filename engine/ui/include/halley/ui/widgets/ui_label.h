#pragma once
#include "../ui_widget.h"
#include "halley/core/graphics/text/text_renderer.h"
#include <climits>

namespace Halley {
	class UILabel : public UIWidget {
	public:
		explicit UILabel(TextRenderer text);

		void setText(const String& text);
		void setColourOverride(const std::vector<ColourOverride>& overrides);
		void setMaxWidth(float maxWidth);

		void setColour(Colour4f colour);

		void draw(UIPainter& painter) const override;
		void update(Time t, bool moved) override;

	private:
		TextRenderer text;
		float maxWidth = std::numeric_limits<float>::infinity();

		void updateMinSize();
	};
}
