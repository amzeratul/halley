#pragma once

#include "../ui_widget.h"
#include "../ui_input_buttons.h"
#include "halley/core/graphics/sprite/sprite.h"
#include "halley/core/graphics/text/text_renderer.h"
#include "ui_clickable.h"

namespace Halley {
	class UIStyle;
	class UIValidator;
	class UIList;
	class UIScrollPane;

	class UIDropdown : public UIClickable {
	public:
		explicit UIDropdown(String id, std::shared_ptr<UIStyle> style, const std::vector<String>& options, int defaultOption = 0);

		void setSelectedOption(int option);
		void setSelectedOption(const String& option);
		int getSelectedOption() const;
		String getSelectedOptionText() const;

		void setInputButtons(const UIInputButtons& buttons);

	protected:
		void draw(UIPainter& painter) const override;
		void drawChildren(UIPainter& painter) const override;
		void update(Time t, bool moved) override;

		void onClicked(Vector2f mousePos) override;
		void doSetState(State state) override;

		bool isFocusLocked() const override;

	private:
		Sprite sprite;
		TextRenderer label;
		UIInputButtons inputButtons;
		std::shared_ptr<UIStyle> style;
		std::shared_ptr<UIWidget> dropdownWindow;
		std::shared_ptr<UIList> dropdownList;
		std::shared_ptr<UIScrollPane> scrollPane;
		
		std::vector<String> options;
		int curOption = 0;
		bool isOpen = false;

		void open();
		void close();
		void scrollToShow(int option, bool center);
    };
}
