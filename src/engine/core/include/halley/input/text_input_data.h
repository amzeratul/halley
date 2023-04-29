#pragma once

#include "halley/maths/range.h"
#include "halley/text/halleystring.h"
#include "halley/data_structures/maybe.h"

namespace Halley {
	enum class KeyMods : uint8_t;
	class IClipboard;
	struct KeyboardKeyPress;

	class TextInputData {
	public:
		struct Selection {
			int start = 0;
			int end = 0;
			bool anchorAtStart = false;

			Selection(int start = 0, int end = 0, bool anchorAtStart = false)
				: start(start)
				, end(end)
				, anchorAtStart(anchorAtStart)
			{}

			Selection(Range<int> range)
				: start(range.start)
				, end(range.end)
				, anchorAtStart(false)
			{}

			Range<int> toRange() const
			{
				return Range<int>(start, end);
			}

			int getAnchor() const
			{
				return anchorAtStart ? start : end;
			}

			int getCaret() const
			{
				return anchorAtStart ? end : start;
			}

			static Selection fromAnchorAndCaret(int anchor, int caret)
			{
				if (anchor < caret) {
					return Selection(anchor, caret, true);
				} else {
					return Selection(caret, anchor, false);
				}
			}
		};

		explicit TextInputData();
		explicit TextInputData(const String& str);
		explicit TextInputData(StringUTF32 str);

		const StringUTF32& getText() const;
		void setText(const String& text);
		void setTextFromSoftKeyboard(const String& text, bool accept);
		void setText(StringUTF32 text);

		Selection getSelection() const;
		void setSelection(int selection);
		void setSelection(Selection selection);
		void moveCursor(int position, KeyMods mods);

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
		void setCaptureSubmit(bool enable);

	private:
		StringUTF32 text;
		Selection selection;

		int minLength = 0;
		std::optional<int> maxLength = {};
		int textRevision = 0;
		bool readOnly = false;
		bool pendingSubmit = false;
		bool captureSubmit = false;

		void onTextModified();
		void onDelete(bool wholeWord = false);
		void onBackspace(bool wholeWord = false);
		void changeSelection(int dir, KeyMods mods);
		int getWordBoundary(int cursorPos, int dir) const;
	};

}
