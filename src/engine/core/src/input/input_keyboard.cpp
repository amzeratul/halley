#include <halley/core/input/input_keyboard.h>
#include "input/input_keys.h"
#include "input/text_input_capture.h"
#include "input/text_input_data.h"
using namespace Halley;

KeyboardKeyPress::KeyboardKeyPress(KeyCode key, KeyMods mod)
	: key(key)
	, mod(mod)
{
}

bool KeyboardKeyPress::operator==(const KeyboardKeyPress& other) const
{
	return key == other.key && mod == other.mod;
}

bool KeyboardKeyPress::is(KeyCode key, KeyMods mod) const
{
	return this->key == key && this->mod == mod;
}

bool KeyboardKeyPress::isPrintable() const
{
	return static_cast<int>(key) >= 32 && static_cast<int>(key) < 128 && (mod == KeyMods::None || mod == KeyMods::Shift);
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

void InputKeyboard::onKeyPressed(KeyCode code, KeyMods mods)
{
	const auto key = KeyboardKeyPress(code, mods);
	
	if (!sendKeyPress(key)) {
		keyPresses.push_back(key);
	}
	
	onButtonPressed(static_cast<int>(code));
}

void InputKeyboard::onKeyReleased(KeyCode code, KeyMods mods)
{
	onButtonReleased(static_cast<int>(code));
}

gsl::span<const KeyboardKeyPress> InputKeyboard::getPendingKeys() const
{
	return { keyPresses };
}

void InputKeyboard::onTextEntered(const char* text)
{
	const auto str = String(text).getUTF32();
	for (const auto& c: captures) {
		c->onTextEntered(str);
	}
}

bool InputKeyboard::sendKeyPress(KeyboardKeyPress chr)
{
	for (const auto& c: captures) {
		const bool handled = c->onKeyPress(chr, clipboard.get());
		if (handled) {
			return true;
		}
	}

	return false;
}

void InputKeyboard::onButtonsCleared()
{
	keyPresses.clear();
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
