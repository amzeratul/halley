#include "winrt_keyboard.h"
#include <halley/core/input/input_keys.h>

using namespace Halley;

using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::System;

Maybe<int> WinRTKeyboard::getHalleyKey(VirtualKey virtualKey)
{
	int code = int(virtualKey);
	if (code < 0 || code >= int(keyMapping.size())) {
		return {};
	}

	int value = keyMapping[code];
	if (value == -1) {
		return {};
	} else {
		return value;
	}
}

WinRTTextInputCapture::WinRTTextInputCapture(WinRTKeyboard& parent)
	: parent(parent)
{
	parent.addCapture(this);
}

WinRTTextInputCapture::~WinRTTextInputCapture()
{
	parent.removeCapture(this);
}

WinRTKeyboard::WinRTKeyboard()
	: InputKeyboard(Keys::Last)
{
	initMapping();

	window = CoreWindow::GetForCurrentThread();
	
	keyDownEvent = window->KeyDown([=] (CoreWindow window, const KeyEventArgs& args)
	{
		auto key = getHalleyKey(args.VirtualKey());
		if (key) {
			onButtonPressed(key.get());
		}
		args.Handled(key.is_initialized());
	});

	keyUpEvent = window->KeyDown([=] (CoreWindow window, const KeyEventArgs& args)
	{
		auto key = getHalleyKey(args.VirtualKey());
		if (key) {
			onButtonReleased(key.get());
		}
		args.Handled(key.is_initialized());
	});
}

WinRTKeyboard::~WinRTKeyboard()
{
	window->KeyDown(keyDownEvent);
	window->KeyUp(keyUpEvent);
}

void WinRTKeyboard::update()
{
}

void WinRTKeyboard::addCapture(WinRTTextInputCapture* capture)
{
	captures.insert(capture);
}

void WinRTKeyboard::removeCapture(WinRTTextInputCapture* capture)
{
	captures.erase(capture);
}

void WinRTKeyboard::onTextControlCharacterGenerated(TextControlCharacter chr)
{
	for (auto& c: captures) {
		c->onControlCharacter(chr);
	}
}

void WinRTKeyboard::initMapping()
{
	keyMapping.resize(256, -1);

	auto set = [&] (VirtualKey key, int value)
	{
		keyMapping[int(key)] = value;
	};

	set(VirtualKey::Enter, Keys::Enter);
	set(VirtualKey::Escape, Keys::Esc);
	set(VirtualKey::Tab, Keys::Tab);
	set(VirtualKey::Space, Keys::Space);
	set(VirtualKey::Delete, Keys::Delete);
	set(VirtualKey::Back, Keys::Backspace);
	
	set(VirtualKey::Up, Keys::Up);
	set(VirtualKey::Down, Keys::Down);
	set(VirtualKey::Left, Keys::Left);
	set(VirtualKey::Right, Keys::Right);

	set(VirtualKey::GamepadDPadUp, Keys::Up);
	set(VirtualKey::GamepadDPadDown, Keys::Down);
	set(VirtualKey::GamepadDPadLeft, Keys::Left);
	set(VirtualKey::GamepadDPadRight, Keys::Right);

	set(VirtualKey::Home, Keys::Home);
	set(VirtualKey::End, Keys::End);
	set(VirtualKey::PageUp, Keys::PageUp);
	set(VirtualKey::PageDown, Keys::PageDown);

	set(VirtualKey::LeftControl, Keys::LCtrl);
	set(VirtualKey::RightControl, Keys::RCtrl);
	set(VirtualKey::LeftShift, Keys::LShift);
	set(VirtualKey::RightShift, Keys::RShift);
	set(VirtualKey::LeftMenu, Keys::LAlt);
	set(VirtualKey::RightMenu, Keys::LAlt);

	set(VirtualKey::F1, Keys::F1);
	set(VirtualKey::F2, Keys::F2);
	set(VirtualKey::F3, Keys::F3);
	set(VirtualKey::F4, Keys::F4);
	set(VirtualKey::F5, Keys::F5);
	set(VirtualKey::F6, Keys::F6);
	set(VirtualKey::F7, Keys::F7);
	set(VirtualKey::F8, Keys::F8);
	set(VirtualKey::F9, Keys::F9);
	set(VirtualKey::F10, Keys::F10);
	set(VirtualKey::F11, Keys::F11);
	set(VirtualKey::F12, Keys::F12);

	for (int i = 0; i < 26; ++i) {
		set(VirtualKey(int(VirtualKey::A) + i), Keys::A + i);
	}

	for (int i = 0; i < 10; ++i) {
		set(VirtualKey(int(VirtualKey::Number0) + i), Keys::Num0 + i);
	}
}

std::unique_ptr<ITextInputCapture> WinRTKeyboard::makeTextInputCapture()
{
	return std::make_unique<WinRTTextInputCapture>(*this);
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

void WinRTTextInputCapture::onControlCharacter(TextControlCharacter chr)
{
	using namespace winrt::Windows::UI::Text::Core;

	auto oldText = input->getText();

	CoreTextRange fullRange;
	fullRange.StartCaretPosition = 0;
	fullRange.EndCaretPosition = int(input->getText().size());

	CoreTextRange oldSel;
	oldSel.StartCaretPosition = input->getSelection().start;
	oldSel.EndCaretPosition = input->getSelection().end;

	input->onControlCharacter(chr, {});

	CoreTextRange newSel;
	newSel.StartCaretPosition = input->getSelection().start;
	newSel.EndCaretPosition = input->getSelection().end;

	if (oldText != input->getText()) {
		editContext->NotifyTextChanged(fullRange, int32_t(input->getText().size()), newSel);
	} else if (oldSel != newSel) {
		editContext->NotifySelectionChanged(newSel);
	}
}
