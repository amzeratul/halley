#pragma once

#include "halley/maths/range.h"
#include "halley/text/halleystring.h"
#include "halley/data_structures/maybe.h"

namespace Halley {
	class IClipboard;
	struct KeyboardKeyPress;

	class TextInputData {
	public:
		explicit TextInputData();
		explicit TextInputData(const String& str);
		explicit TextInputData(StringUTF32 str);

		const StringUTF32& getText() const;
		void setText(const String& text);
		void setTextFromSoftKeyboard(const String& text, bool accept);
		void setText(StringUTF32 text);

		Range<int> getSelection() const;
		void setSelection(int selection);
		void setSelection(Range<int> selection);

		void setLengthLimits(int min, std::optional<int> max);
		int getMinLength() const;
		std::optional<int> getMaxLength() const;

		void insertText(const String& text);
		void insertText(const StringUTF32& text);

		bool onKeyPress(KeyboardKeyPress c, IClipboard* clipboard);

		int getTextRevision() const;
		Range<int> getTotalRange() const;

		void setReadOnly(bool enable);
		bool isReadOnly() const;

		bool isPendingSubmit();

	private:
		StringUTF32 text;
		Range<int> selection;

		int minLength = 0;
		std::optional<int> maxLength = {};
		int textRevision = 0;
		bool readOnly = false;
		bool pendingSubmit = false;

		void onTextModified();
		void onDelete();
		void onBackspace();
	};

}
