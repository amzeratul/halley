#pragma once
#include "halley/core/api/halley_api_internal.h"
#include "halley/core/input/input_keyboard.h"
#include <winrt/Windows.UI.Text.Core.h>

namespace Halley
{
	class WinRTInput : public InputAPIInternal
	{
	public:
		void init() override;
		void deInit() override;
		void beginEvents(Time t) override;

		size_t getNumberOfKeyboards() const override;
		std::shared_ptr<InputKeyboard> getKeyboard(int id) const override;

		size_t getNumberOfJoysticks() const override;
		std::shared_ptr<InputJoystick> getJoystick(int id) const override;

		size_t getNumberOfMice() const override;
		std::shared_ptr<InputDevice> getMouse(int id) const override;
		void setMouseRemapping(std::function<Vector2f(Vector2i)> remapFunction) override;

		Vector<std::shared_ptr<InputTouch>> getNewTouchEvents() override;
		Vector<std::shared_ptr<InputTouch>> getTouchEvents() override;

	private:
		std::vector<std::shared_ptr<InputJoystick>> gamepads;
		std::shared_ptr<InputKeyboard> keyboard;
	};

	class WinRTKeyboard : public InputKeyboard {
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
