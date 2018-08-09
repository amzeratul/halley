#include "winrt_keyboard.h"
#include <winrt/Windows.UI.Text.h>

using namespace Halley;


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
