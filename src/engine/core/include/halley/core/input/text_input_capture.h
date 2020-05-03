#pragma once

#include "halley/text/halleystring.h"

namespace Halley {
	struct KeyboardKeyPress;
	class InputKeyboard;
	class TextInputData;
	class IClipboard;

	struct SoftwareKeyboardData {
		String title;
		String subText;
		String guideText;
	};

	class ITextInputCapture {
	public:
		virtual ~ITextInputCapture() = default;

		virtual void open(TextInputData& input, SoftwareKeyboardData softKeyboardData) = 0;
		virtual void close() = 0;

		virtual bool isOpen() const = 0;
		virtual void update() = 0;

		virtual void onTextEntered(const StringUTF32& text);
		virtual bool onKeyPress(KeyboardKeyPress c, IClipboard* clipboard);
	};

	class StandardTextInputCapture final : public ITextInputCapture {
	public:
		StandardTextInputCapture(InputKeyboard& parent);
		~StandardTextInputCapture();

		void open(TextInputData& input, SoftwareKeyboardData softKeyboardData) override;
		void close() override;
		bool isOpen() const override;
		void update() override;

		void onTextEntered(const StringUTF32& text) override;
		bool onKeyPress(KeyboardKeyPress c, IClipboard* clipboard) override;

	private:
		bool currentlyOpen = false;
		InputKeyboard& parent;
		TextInputData* textInput;
	};

	class TextInputCapture {
	public:
		TextInputCapture(TextInputData& inputData, SoftwareKeyboardData data, std::unique_ptr<ITextInputCapture> capture);
		~TextInputCapture();

		TextInputCapture(TextInputCapture&& other) noexcept = default;
		TextInputCapture(const TextInputCapture& other) = delete;
		TextInputCapture& operator=(TextInputCapture&& other) noexcept = default;
		TextInputCapture& operator=(const TextInputCapture& other) = delete;

		bool update() const; // Returns if the capture is still open

	private:
		std::unique_ptr<ITextInputCapture> capture;
	};
}
