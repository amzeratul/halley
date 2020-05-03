#include <halley/core/input/input_keyboard.h>
#include "input/input_keys.h"
#include "input/text_input_capture.h"
#include "input/text_input_data.h"
using namespace Halley;

KeyboardKeyPress::KeyboardKeyPress(Keys key, KeyMods mod)
	: key(key)
	, mod(mod)
{
}

bool KeyboardKeyPress::operator==(const KeyboardKeyPress& other) const
{
	return key == other.key && mod == other.mod;
}

bool KeyboardKeyPress::is(Keys key, KeyMods mod) const
{
	return this->key == key && this->mod == mod;
}

InputKeyboard::InputKeyboard(int nButtons, std::shared_ptr<IClipboard> clipboard)
	: InputButtonBase(nButtons)
	, clipboard(std::move(clipboard))
{
}

TextInputCapture InputKeyboard::captureText(TextInputData& textInputData, SoftwareKeyboardData data)
{
	return TextInputCapture(textInputData, std::move(data), makeTextInputCapture());
}

void InputKeyboard::onButtonPressed(int scanCode)
{
	KeyMods mods = KeyMods::None;
	if (isButtonDown(Keys::LShift) || isButtonDown(Keys::RShift)) {
		mods = KeyMods(static_cast<int>(mods) | static_cast<int>(KeyMods::Shift));
	}
	if (isButtonDown(Keys::LCtrl) || isButtonDown(Keys::RCtrl)) {
		mods = KeyMods(static_cast<int>(mods) | static_cast<int>(KeyMods::Ctrl));
	}
	if (isButtonDown(Keys::LAlt) || isButtonDown(Keys::RAlt)) {
		mods = KeyMods(static_cast<int>(mods) | static_cast<int>(KeyMods::Alt));
	}

	onKeyPress({ Keys(scanCode), mods });

	InputButtonBase::onButtonPressed(scanCode);
}

void InputKeyboard::onButtonReleased(int scanCode)
{
	InputButtonBase::onButtonReleased(scanCode);
}

void InputKeyboard::onTextEntered(const char* text)
{
	const auto str = String(text).getUTF32();
	for (const auto& c: captures) {
		c->onTextEntered(str);
	}
}

bool InputKeyboard::onKeyPress(KeyboardKeyPress chr)
{
	for (const auto& c: captures) {
		const bool handled = c->onKeyPress(chr, clipboard.get());
		if (handled) {
			return true;
		}
	}

	return false;
}

std::unique_ptr<ITextInputCapture> InputKeyboard::makeTextInputCapture()
{
	auto ptr = std::make_unique<StandardTextInputCapture>(*this);
	captures.insert(ptr.get());
	return ptr;
}

void InputKeyboard::removeCapture(ITextInputCapture* capture)
{
	captures.erase(capture);
}
