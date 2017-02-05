#pragma once

#include "../ui_widget.h"
#include "halley/core/graphics/sprite/sprite.h"
#include "halley/core/graphics/text/text_renderer.h"
#include "halley/core/input/input_device.h"

namespace Halley {
	class UIStyle;
	class UIValidator;

	class UIInput : public UIWidget {
	public:
		explicit UIInput(std::shared_ptr<InputDevice> keyboard, String id, std::shared_ptr<UIStyle> style, String text = "", String ghostText = "");
		bool isFocusable() const override;

		UIInput& setText(const String& text);
		String getText() const;
		UIInput& setGhostText(const String& text);
		String getGhostText() const;

		void setValidator(std::shared_ptr<UIValidator> validator);

	protected:
		void draw(UIPainter& painter) const override;
		void update(Time t, bool moved) override;

		void onFocus() override;

	private:
		void updateTextInput();

		std::shared_ptr<InputDevice> keyboard;
		std::shared_ptr<UIStyle> style;
		Sprite sprite;
		TextRenderer label;

		std::shared_ptr<UIValidator> validator;

		StringUTF32 text;
		String ghostText;

		float caretTime = 0;
		bool caretShowing = false;
	};
}
