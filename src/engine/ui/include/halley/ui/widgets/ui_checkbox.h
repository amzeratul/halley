#pragma once
#include "ui_button.h"

namespace Halley {
	class UIStyle;

	class UICheckbox : public UIClickable {
	public:
		explicit UICheckbox(String id, UIStyle style, bool checked = false);

		void draw(UIPainter& painter) const override;
		void update(Time t, bool moved) override;

        bool isChecked() const;
        void setChecked(bool checked);

		void onClicked(Vector2f mousePos) override;
		void doSetState(State state) override;

		void onManualControlCycleValue(int delta) override;
		void onManualControlActivate() override;

		void readFromDataBind() override;

	private:
		Sprite sprite;
		UIStyle style;
        bool checked = false;
	};
}
