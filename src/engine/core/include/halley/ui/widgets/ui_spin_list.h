#pragma once

#include "../ui_widget.h"
#include "halley/graphics/sprite/sprite.h"
#include "ui_image.h"

namespace Halley {
    class UILabel;
    class UISpinListArrow;
    class I18N;
	class UIStyle;
	class UIValidator;
	class UIList;
	class UIScrollPane;

	class UISpinList : public UIWidget {
		friend UISpinListArrow;
	public:
		explicit UISpinList(String id, const UIStyle& style, Vector<LocalisedString> options = {}, int defaultOption = 0);

		void setSelectedOptionSilent(int option);
		void setSelectedOption(int option);
		void setSelectedOption(const String& id);
		int getSelectedOption() const;
		String getSelectedOptionId() const;
		LocalisedString getSelectedOptionText() const;

		void setInputButtons(const UIInputButtons& buttons) override;
		void setOptions(Vector<LocalisedString> options, int defaultOption = -1);
		void setOptions(Vector<String> optionIds, const Vector<LocalisedString>& options, int defaultOption = -1);
		void setOptions(const I18N& i18n, const String& i18nPrefix, const Vector<String>& optionIds, int defaultOption = -1);
		void setMinMax(int min, int max);

		void onManualControlCycleValue(int delta) override;
		bool canReceiveFocus() const override;

	protected:
		void update(Time t, bool moved) override;
		void readFromDataBind() override;

	private:
		void arrowPressed(bool left);
	    UIInputButtons inputButtons;

		std::shared_ptr<UILabel> label;
		std::shared_ptr<UISpinListArrow> leftArrow;
		std::shared_ptr<UISpinListArrow> rightArrow;

		Vector<LocalisedString> options;
		Vector<String> optionIds;
		int curOption = 0;
	};

	class UISpinListArrow : public UIImage {
	public:
		explicit UISpinListArrow(UISpinList& parent, String id, const UIStyle& style, bool left);

	protected:
		void update(Time t, bool moved) override;
		void pressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;
		
	private:
		UISpinList& parent;
		bool left = false;
		float time = 999.0;

		bool wasHovering = false;
	    float hoverTime = 0.0f;
	};
}
