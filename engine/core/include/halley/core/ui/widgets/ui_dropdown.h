#pragma once

#include "../ui_widget.h"
#include "halley/core/graphics/sprite/sprite.h"
#include "halley/core/graphics/text/text_renderer.h"
#include "ui_button.h"

namespace Halley {
	class UIStyle;
	class UIValidator;

	class UIDropdown : public UIClickable {
	public:
		explicit UIDropdown(String id, std::shared_ptr<UIStyle> style, const std::vector<String>& options, int defaultOption = 0);

		void setSelectedOption(int option);
		int getSelectedOption() const;

	protected:
		void draw(UIPainter& painter) const override;
		void update(Time t, bool moved) override;

		void onClicked(Vector2f mousePos) override;
		void doSetState(State state) override;

		bool isFocusLocked() const override;
		void onFocusLost() override;

		Rect4f getMouseRect() const override;

	private:
		Sprite sprite;
		Sprite dropdownSprite;
		TextRenderer label;
		std::shared_ptr<UIStyle> style;

		int curOptionHighlight = -1;
		Vector2f optionsExtent;
		std::vector<TextRenderer> optionsLabels;

		std::vector<String> options;
		int curOption = 0;
		bool isOpen = false;
    };
}
