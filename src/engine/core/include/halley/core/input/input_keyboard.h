#pragma once

#include "input_button_base.h"
#include "halley/data_structures/maybe.h"
#include "halley/maths/range.h"

namespace Halley {
	class TextInputData {
	public:
		explicit TextInputData();
		explicit TextInputData(const String& str);
		explicit TextInputData(StringUTF32 str);

		const StringUTF32& getText() const;
		void setText(StringUTF32 text);

		Range<int> getSelection() const;
		void setSelection(int selection);
		void setSelection(Range<int> selection);

		void setLengthLimits(int min, Maybe<int> max);
		int getMinLength() const;
		Maybe<int> getMaxLength() const;

		void insertText(const String& text);
		void insertText(const StringUTF32& text);
		
		void onDelete();
		void onBackspace();

	private:
		StringUTF32 text;
		Range<int> selection;

		int minLength = 0;
		Maybe<int> maxLength = {};

		void onTextModified();
	};

	struct SoftwareKeyboardData {
		String title;
		String subText;
		String guideText;
	};

	class ITextInputCapture {
	public:
		virtual ~ITextInputCapture() = default;
		
		virtual void open(const TextInputData& input, SoftwareKeyboardData softKeyboardData) = 0;
		virtual void close() = 0;

		virtual bool isOpen() const = 0;
		virtual void update(TextInputData& input) = 0;
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
		TextInputData& inputData;
		std::unique_ptr<ITextInputCapture> capture;
	};

	class InputKeyboard : public InputButtonBase {
	public:
		explicit InputKeyboard(int nButtons = -1);
		virtual ~InputKeyboard() = default;

		virtual TextInputCapture captureText(TextInputData& textInputData, SoftwareKeyboardData softKeyboardData);

	protected:
		virtual std::unique_ptr<ITextInputCapture> makeTextInputCapture() = 0;
	};
}
