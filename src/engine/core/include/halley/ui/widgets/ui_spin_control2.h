#pragma once
#include "ui_textinput.h"

namespace Halley {
	class UIButton;

	class UISpinControl2 : public UITextInput {
	public:
		explicit UISpinControl2(String id, UIStyle style, float value, bool allowFloat);

		UISpinControl2(UISpinControl2&& other) = delete;
		UISpinControl2(const UISpinControl2& other) = delete;
		UISpinControl2& operator=(UISpinControl2&& other) = delete;
		UISpinControl2& operator=(const UISpinControl2& other) = delete;

		void setValue(float value);
		float getValue() const;

		void setIncrement(float inc);

		void setMinimumValue(std::optional<float> value);
		void setMaximumValue(std::optional<float> value);
		
		void onManualControlCycleValue(int delta) override;

		void readFromDataBind() override;

	protected:
		Vector4f getTextInnerBorder() const override;
		bool onKeyPress(KeyboardKeyPress key) override;

	private:
		float increment = 1;
		std::optional<float> minValue;
		std::optional<float> maxValue;
    };
}
