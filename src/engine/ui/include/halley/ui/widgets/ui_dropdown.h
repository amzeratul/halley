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
		class Entry {
		public:
			String id;
			LocalisedString label;
			Sprite icon;

			Entry() = default;
			Entry(String id, LocalisedString label, Sprite icon = Sprite())
				: id(std::move(id))
				, label(std::move(label))
				, icon(std::move(icon))
			{}
		};
		
		UIDropdown(String id, UIStyle style, std::vector<LocalisedString> options = {}, int defaultOption = 0);

		virtual void setSelectedOption(int option);
		virtual void setSelectedOption(const String& id);
		int getSelectedOption() const;
		String getSelectedOptionId() const;
		LocalisedString getSelectedOptionText() const;
		int getNumberOptions() const;

		void setInputButtons(const UIInputButtons& buttons) override;
		void setOptions(std::vector<LocalisedString> options, int defaultOption = -1);
		void setOptions(std::vector<String> optionsIds, int defaultOption = -1);
		void setOptions(std::vector<String> optionIds, std::vector<LocalisedString> options, int defaultOption = -1);
		void setOptions(std::vector<Entry> options, int defaultOption = -1);
		void setOptions(const I18N& i18n, const String& i18nPrefix, std::vector<String> optionIds, int defaultOption = -1);

		void onManualControlCycleValue(int delta) override;
		void onManualControlActivate() override;

		bool canReceiveFocus() const override;

	protected:		
		std::vector<Entry> options;
		
		TextRenderer label;
		Sprite icon;
		std::shared_ptr<UIList> dropdownList;
		
		int curOption = 0;

		void draw(UIPainter& painter) const override;
		void update(Time t, bool moved) override;

		void onClicked(Vector2f mousePos, KeyMods keyMods) override;
		void doSetState(State state) override;

		bool isFocusLocked() const override;

		void readFromDataBind() override;

		virtual void updateOptionLabels();

	private:
		enum class OpenState : uint8_t {
			Closed,
			OpenDown,
			OpenUp
		};
		
		Sprite sprite;
		UIInputButtons inputButtons;
		std::shared_ptr<UIWidget> dropdownWindow;
		std::shared_ptr<UIScrollPane> scrollPane;
		
		OpenState openState = OpenState::Closed;

		void open();
		void close();
    };
}
