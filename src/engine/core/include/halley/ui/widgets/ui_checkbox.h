#pragma once
#include "ui_button.h"

namespace Halley {
	class UIStyle;

	class UICheckbox : public UIClickable {
	public:
		explicit UICheckbox(String id, UIStyle style, bool checked = false, LocalisedString label = {});

		void update(Time t, bool moved) override;

        bool isChecked() const;
        void setChecked(bool checked);

		void onClicked(Vector2f mousePos, KeyMods keyMods) override;
		void doSetState(State state) override;

		void onManualControlCycleValue(int delta) override;
		void onManualControlActivate() override;

		void readFromDataBind() override;

		void setLabel(LocalisedString str);

	private:
		std::shared_ptr<UIImage> image;
		std::shared_ptr<UILabel> label;
        bool checked = false;
	};
}
