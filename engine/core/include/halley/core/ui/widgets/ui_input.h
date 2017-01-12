#pragma once

#include "../ui_widget.h"
#include "halley/core/graphics/sprite/sprite.h"
#include "halley/core/graphics/text/text_renderer.h"

namespace Halley {
	class UIStyle;

	class UIInput : public UIWidget {
	public:
		explicit UIInput(String id, std::shared_ptr<UIStyle> style, String text = "", String ghostText = "");
		bool isFocusable() const override;

		UIInput& setText(const String& text);
		String getText() const;
		UIInput& setGhostText(const String& text);
		String getGhostText() const;

	protected:
		void draw(UIPainter& painter) const override;
		void update(Time t, bool moved) override;

	private:
		std::shared_ptr<UIStyle> style;
		Sprite sprite;
		TextRenderer label;

		String text;
		String ghostText;
	};
}
