#pragma once

#include "../ui_widget.h"
#include "halley/core/graphics/sprite/sprite.h"
#include "halley/core/graphics/text/text_renderer.h"
#include "ui_clickable.h"

namespace Halley {
	class I18N;
	class UIStyle;
	class UIValidator;
	class UIList;
	class UIScrollPane;

	class UIDropdown : public UIClickable {
	public:
		explicit UIDropdown(String id, UIStyle style, UIStyle scrollbarStyle, UIStyle listStyle, std::vector<LocalisedString> options = {}, int defaultOption = 0);

		void setSelectedOption(int option);
		void setSelectedOption(const String& id);
		int getSelectedOption() const;
		String getSelectedOptionId() const;
		LocalisedString getSelectedOptionText() const;

		void setInputButtons(const UIInputButtons& buttons) override;
		void setOptions(std::vector<LocalisedString> options, int defaultOption = -1);
		void setOptions(std::vector<String> optionIds, std::vector<LocalisedString> options, int defaultOption = -1);
		void setOptions(const I18N& i18n, const String& i18nPrefix, std::vector<String> optionIds, int defaultOption = -1);

		void onManualControlCycleValue(int delta) override;
		void onManualControlActivate() override;

	protected:
		void draw(UIPainter& painter) const override;
		void drawChildren(UIPainter& painter) const override;
		void update(Time t, bool moved) override;

		void onClicked(Vector2f mousePos) override;
		void doSetState(State state) override;

		bool isFocusLocked() const override;

		void readFromDataBind() override;

	private:
		Sprite sprite;
		TextRenderer label;
		UIInputButtons inputButtons;
		UIStyle style;
		UIStyle scrollbarStyle;
		UIStyle listStyle;
		std::shared_ptr<UIWidget> dropdownWindow;
		std::shared_ptr<UIList> dropdownList;
		std::shared_ptr<UIScrollPane> scrollPane;
		
		std::vector<LocalisedString> options;
		std::vector<String> optionIds;
		int curOption = 0;
		bool isOpen = false;

		void open();
		void close();
		void updateOptionLabels();
    };
}
