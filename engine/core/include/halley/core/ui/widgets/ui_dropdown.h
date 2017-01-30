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

		void onClicked() override;
		void doSetState(State state) override;

		bool isFocusLocked() const override;

	private:
		Sprite sprite;
		TextRenderer label;
		std::shared_ptr<UIStyle> style;

		std::vector<String> options;
		int curOption = 0;
		bool isOpen = false;
    };
}
