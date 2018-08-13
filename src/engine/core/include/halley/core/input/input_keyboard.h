#pragma once

#include "input_button_base.h"
#include "halley/data_structures/maybe.h"
#include "halley/maths/range.h"

namespace Halley {
	enum class TextControlCharacter {
		Enter,
		Delete,
		Backspace,
		Left,
		Right,
		Up,
		Down,
		PageUp,
		PageDown,
		Home,
		End,
		Tab,
		Copy,
		Paste,
		Cut,
		Undo,
		Redo,
		SelectAll,
		SelectLeft,
		SelectRight,
		SelectUp,
		SelectDown
	};

	class IClipboard;

	class TextInputData {
	public:
		explicit TextInputData();
		explicit TextInputData(const String& str);
		explicit TextInputData(StringUTF32 str);

		const StringUTF32& getText() const;
		void setText(const String& text);
		void setText(StringUTF32 text);

		Range<int> getSelection() const;
		void setSelection(int selection);
		void setSelection(Range<int> selection);

		void setLengthLimits(int min, Maybe<int> max);
		int getMinLength() const;
		Maybe<int> getMaxLength() const;

		void insertText(const String& text);
		void insertText(const StringUTF32& text);
		
		void onControlCharacter(TextControlCharacter c, std::shared_ptr<IClipboard> clipboard);

		int getTextRevision() const;
		Range<int> getTotalRange() const;

	private:
		StringUTF32 text;
		Range<int> selection;

		int minLength = 0;
		Maybe<int> maxLength = {};
		int textRevision = 0;

		void onTextModified();
		void onDelete();
		void onBackspace();
	};

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
	};

	class TextInputCapture {
	public:
		TextInputCapture(TextInputData& inputData, SoftwareKeyboardData data, std::unique_ptr<ITextInputCapture> capture);
		~TextInputCapture();

		TextInputCapture(TextInputCapture&& other) = default;
		TextInputCapture(const TextInputCapture& other) = delete;
		TextInputCapture& operator=(TextInputCapture&& other) = default;
		TextInputCapture& operator=(const TextInputCapture& other) = delete;

		bool update() const; // Returns if the capture is still open

	private:
		std::unique_ptr<ITextInputCapture> capture;
	};

	class InputKeyboard : public InputButtonBase {
	public:
		explicit InputKeyboard(int nButtons = -1);
		virtual ~InputKeyboard() = default;

		virtual TextInputCapture captureText(TextInputData& textInputData, SoftwareKeyboardData softKeyboardData);
		
		void onButtonPressed(int scanCode) override;
		void onButtonReleased(int scanCode) override;
		virtual void onTextControlCharacterGenerated(TextControlCharacter c);

	protected:
		virtual std::unique_ptr<ITextInputCapture> makeTextInputCapture() = 0;
	};
}
