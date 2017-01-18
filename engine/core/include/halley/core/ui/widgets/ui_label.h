#pragma once
#include "../ui_widget.h"
#include "halley/core/graphics/text/text_renderer.h"

namespace Halley {
	class UILabel : public UIWidget {
	public:
		explicit UILabel(TextRenderer text);

		void setText(const String& text);

		void draw(UIPainter& painter) const override;
		void update(Time t, bool moved) override;

	private:
		TextRenderer text;
	};
}
