#pragma once
#include "ui_button.h"

namespace Halley {
	class UIStyle;

	class UICheckbox : public UIButton {
	public:
		explicit UICheckbox(String id, std::shared_ptr<UIStyle> style, bool checked = false);

        bool isChecked() const;
        void setChecked(bool checked);

		void onClicked() override;
		bool setState(State state) override;

	private:
        bool checked = false;
	};
}
