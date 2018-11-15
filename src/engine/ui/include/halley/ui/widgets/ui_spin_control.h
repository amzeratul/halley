#pragma once
#include "halley/ui/ui_widget.h"
#include "ui_textinput.h"

namespace Halley {
	class UIButton;

	class UISpinControl : public UIWidget {
	public:
		explicit UISpinControl(std::shared_ptr<InputKeyboard> keyboard, String id, UIStyle style, int value);

		UISpinControl(UISpinControl&& other) = default;
		UISpinControl(const UISpinControl& other) = delete;
		UISpinControl& operator=(UISpinControl&& other) = default;
		UISpinControl& operator=(const UISpinControl& other) = delete;

		void setValue(int value);
		int getValue() const;

		void setMinimumValue(Maybe<int> value);
		void setMaximumValue(Maybe<int> value);

		void onManualControlActivate() override;
		void onManualControlCycleValue(int delta) override;

		void readFromDataBind() override;

	private:
		std::shared_ptr<UITextInput> textInput;
		int value;
		Maybe<int> minValue;
		Maybe<int> maxValue;
    };
}
