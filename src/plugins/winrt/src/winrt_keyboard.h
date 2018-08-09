#pragma once

#include "halley/core/input/input_keyboard.h"
#include <winrt/Windows.UI.Text.Core.h>

namespace Halley {
	class WinRTKeyboard : public InputKeyboard {
	public:
		void update();

	protected:
		std::unique_ptr<ITextInputCapture> makeTextInputCapture() override;
	};

	class WinRTTextInputCapture : public ITextInputCapture {
	public:
		void open(TextInputData& input, SoftwareKeyboardData softKeyboardData) override;
		void close() override;
		bool isOpen() const override;
		void update() override;

	private:
		TextInputData* input = nullptr;
		Maybe<winrt::Windows::UI::Text::Core::CoreTextServicesManager> servicesManager;
		Maybe<winrt::Windows::UI::Text::Core::CoreTextEditContext> editContext;
	};	
}
