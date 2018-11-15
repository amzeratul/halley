#pragma once
#include "halley/ui/ui_widget.h"
#include "ui_textinput.h"

namespace Halley {
	class UIButton;

	class UISpinControl : public UIWidget {
	public:
		explicit UISpinControl(std::shared_ptr<InputKeyboard> keyboard, String id, UIStyle style, float value);

		UISpinControl(UISpinControl&& other) = default;
		UISpinControl(const UISpinControl& other) = delete;
		UISpinControl& operator=(UISpinControl&& other) = default;
		UISpinControl& operator=(const UISpinControl& other) = delete;

		void setValue(float value);
		float getValue() const;

		void setIncrement(float inc);

		void setMinimumValue(Maybe<float> value);
		void setMaximumValue(Maybe<float> value);

		void onManualControlActivate() override;
		void onManualControlCycleValue(int delta) override;

		void readFromDataBind() override;

	private:
		std::shared_ptr<UITextInput> textInput;
		float value = 0;
		float increment = 1;
		Maybe<float> minValue;
		Maybe<float> maxValue;
    };
}
