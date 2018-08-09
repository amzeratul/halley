#ifdef WINDOWS_STORE

#include "winrt_input.h"
#include "winrt_gamepad.h"
#include <winrt/Windows.UI.Text.h>
using namespace Halley;

void WinRTInput::init()
{
	for (int i = 0; i < 4; ++i) {
		gamepads.push_back(std::make_shared<WinRTGamepad>(i));
	}
	keyboard = std::make_shared<WinRTKeyboard>();
}

void WinRTInput::deInit()
{
	
}

void WinRTInput::beginEvents(Time t)
{
	for (auto& g: gamepads) {
		g->clearPresses();
		g->update(t);
	}
}

size_t WinRTInput::getNumberOfKeyboards() const
{
	return 1;
}

std::shared_ptr<InputKeyboard> WinRTInput::getKeyboard(int id) const
{
	return keyboard;
}

size_t WinRTInput::getNumberOfJoysticks() const
{
	return 4;
}

std::shared_ptr<InputJoystick> WinRTInput::getJoystick(int id) const
{
	return gamepads.at(id);
}

size_t WinRTInput::getNumberOfMice() const
{
	return 0;
}

std::shared_ptr<InputDevice> WinRTInput::getMouse(int id) const
{
	return {};
}

void WinRTInput::setMouseRemapping(std::function<Vector2f(Vector2i)> remapFunction)
{
}

Vector<std::shared_ptr<InputTouch>> WinRTInput::getNewTouchEvents()
{
	return {};
}

Vector<std::shared_ptr<InputTouch>> WinRTInput::getTouchEvents()
{
	return {};
}

std::unique_ptr<ITextInputCapture> WinRTKeyboard::makeTextInputCapture()
{
	return std::make_unique<WinRTTextInputCapture>();
}

void WinRTTextInputCapture::open(TextInputData& input_, SoftwareKeyboardData softKeyboardData)
{
	using namespace winrt::Windows::UI::Text::Core;
	input = &input_;

	servicesManager = CoreTextServicesManager::GetForCurrentView();
	editContext = servicesManager->CreateEditContext();

	editContext->TextRequested([this] (const CoreTextEditContext& ctx, const CoreTextTextRequestedEventArgs& args)
	{
		String text = String(input->getText());
		auto textUTF16 = text.getUTF16();
		args.Request().Text(textUTF16.c_str());
		CoreTextRange sel;
		sel.StartCaretPosition = input->getSelection().start;
		sel.EndCaretPosition = input->getSelection().end;
		args.Request().Range() = sel;
	});

	editContext->TextUpdating([this] (const CoreTextEditContext& ctx, const CoreTextTextUpdatingEventArgs& args)
	{
		input->setSelection(Range<int>(args.Range().StartCaretPosition, args.Range().EndCaretPosition));
		input->insertText(String(args.Text().c_str()));
		input->setSelection(Range<int>(args.NewSelection().StartCaretPosition, args.NewSelection().EndCaretPosition));
		args.Result(CoreTextTextUpdatingResult::Succeeded);
	});

	editContext->SelectionRequested([this] (const CoreTextEditContext& ctx, const CoreTextSelectionRequestedEventArgs& args)
	{
		CoreTextRange sel;
		sel.StartCaretPosition = input->getSelection().start;
		sel.EndCaretPosition = input->getSelection().end;
		args.Request().Selection() = sel;
	});

	editContext->SelectionUpdating([this] (const CoreTextEditContext& ctx, const CoreTextSelectionUpdatingEventArgs& args)
	{
		input->setSelection(Range<int>(args.Selection().StartCaretPosition, args.Selection().EndCaretPosition));
		args.Result(CoreTextSelectionUpdatingResult::Succeeded);
	});

	CoreTextRange newSel;
	newSel.StartCaretPosition = input->getSelection().start;
	newSel.EndCaretPosition = input->getSelection().end;
	editContext->NotifyTextChanged(CoreTextRange(), int32_t(input->getText().size()), newSel);
	editContext->NotifyFocusEnter();
}

void WinRTTextInputCapture::close()
{
	editContext->NotifyFocusLeave();
	editContext = {};
	input = nullptr;
}

bool WinRTTextInputCapture::isOpen() const
{
	return editContext.is_initialized();
}

void WinRTTextInputCapture::update()
{
}

#endif
