#pragma once
#include "ui_button.h"

namespace Halley {
	class UIStyle;

	class UICheckbox : public UIClickable {
	public:
		explicit UICheckbox(String id, std::shared_ptr<UIStyle> style, bool checked = false);

		void draw(UIPainter& painter) const override;
		void update(Time t, bool moved) override;

        bool isChecked() const;
        void setChecked(bool checked);

		void onClicked(Vector2f mousePos) override;
		void doSetState(State state) override;

	private:
		Sprite sprite;
		std::shared_ptr<UIStyle> style;
        bool checked = false;
	};
}
