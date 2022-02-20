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

	class UISpinList : public UIWidget {
	public:
		explicit UISpinList(String id, const UIStyle& style, Vector<LocalisedString> options = {}, int defaultOption = 0);

		void setSelectedOption(int option);
		void setSelectedOption(const String& id);
		int getSelectedOption() const;
		String getSelectedOptionId() const;
		LocalisedString getSelectedOptionText() const;

		void setInputButtons(const UIInputButtons& buttons) override;
		void setOptions(Vector<LocalisedString> options, int defaultOption = -1);
		void setOptions(Vector<String> optionIds, const Vector<LocalisedString>& options, int defaultOption = -1);
		void setOptions(const I18N& i18n, const String& i18nPrefix, const Vector<String>& optionIds, int defaultOption = -1);

		void onManualControlCycleValue(int delta) override;
		void onManualControlActivate() override;

		bool canReceiveFocus() const override;

	protected:
		void draw(UIPainter& painter) const override;
		void drawChildren(UIPainter& painter) const override;
		void update(Time t, bool moved) override;

		void readFromDataBind() override;

	private:
		Sprite sprite;
		UIInputButtons inputButtons;

		std::shared_ptr<UIWidget> spinner;

		Vector<LocalisedString> options;
		Vector<String> optionIds;
		int curOption = 0;

		void updateLabelPositions();
		void updateOptionLabels();
	};
}
