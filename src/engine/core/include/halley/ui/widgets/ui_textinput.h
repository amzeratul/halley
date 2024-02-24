#pragma once

#include "../ui_widget.h"
#include "halley/graphics/sprite/sprite.h"
#include "halley/graphics/text/text_renderer.h"
#include "halley/input/input_device.h"
#include "halley/text/i18n.h"
#include "halley/input/input_keyboard.h"
#include "halley/input/text_input_data.h"

namespace Halley {
	class UIStyle;
	class IClipboard;

	class UITextInput : public UIWidget {
	public:
		using AutoCompleteHandle = std::function<Vector<StringUTF32>(StringUTF32)>;
		
		UITextInput(String id, UIStyle style, String text = "", LocalisedString ghostText = {}, std::shared_ptr<UIValidator> validator = {});

		UITextInput(UITextInput&& other) = delete;
		UITextInput(const UITextInput& other) = delete;
		UITextInput& operator=(UITextInput&& other) = delete;
		UITextInput& operator=(const UITextInput& other) = delete;

		bool canInteractWithMouse() const override;

		UITextInput& setText(const String& text);
		UITextInput& setText(StringUTF32 text);

		String getText() const;
		UITextInput& setGhostText(LocalisedString text);
		UITextInput& setAppendText(LocalisedString text);
		LocalisedString getGhostText() const;
		void setShowGhostWhenFocused(bool show);
		
		TextRenderer& getTextLabel();

		std::optional<int> getMaxLength() const;
		void setMaxLength(std::optional<int> length);

		Range<int> getSelection() const;
		void setSelection(int selection);
		void setSelection(Range<int> selection);
		void selectLast();

		void onManualControlActivate() override;

		void setAutoCompleteHandle(AutoCompleteHandle handle);

		bool canReceiveFocus() const override;

		void setReadOnly(bool enabled);
		bool isReadOnly() const;

		void setHistoryEnabled(bool enabled);
		bool isHistoryEnabled() const;

		void setMultiLine(bool enabled);
		bool isMultiLine() const;

		void setSelectAllOnClick(bool enabled);
		bool isSelectAllOnClick() const;

		void submit();
		void setClearOnSubmit(bool enabled);
		bool isClearOnSubmit() const;

		void setIcon(Sprite icon, Vector4f border);

		void setAutoSize(std::optional<Range<float>> range, bool horizontal = true);

	protected:
		void draw(UIPainter& painter) const override;
		void update(Time t, bool moved) override;

		void onFocus(bool byClicking) override;
		TextInputData* getTextInputData() override;

		bool onKeyPress(KeyboardKeyPress key) override;
		
		void pressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;
		void releaseMouse(Vector2f mousePos, int button) override;
		void onMouseOver(Vector2f mousePos) override;
		bool isFocusLocked() const override;

		void readFromDataBind() override;

		virtual Vector4f getTextInnerBorder() const;

		std::optional<MouseCursorMode> getMouseCursorMode() const override;

	private:
		void updateCaret();
		Rect4f getTextBounds() const;

		void onMaybeTextModified();
		void onTextModified();
		void validateText();
		void onValidatorSet() override;

		void autoComplete();
		void updateAutoCompleteOnTextModified();
		void refreshAutoCompleteOptions();
		StringUTF32 getAutoCompleteCaption() const;

		void addToHistory(String str);
		void navigateHistory(int delta);
		void updateHistoryOnTextModified();

		void drawSelection(Range<int> sel, UIPainter& painter) const;
		void drawSelectionRow(Range<int> row, UIPainter& painter) const;

		Sprite sprite;
		Sprite caret;
		Sprite selectionBg;
		Sprite icon;
		TextRenderer label;
		TextRenderer ghostLabel;
		Vector4f iconBorder;

		TextInputData text;
		LocalisedString ghostText;
		LocalisedString appendText;
		StringUTF32 lastText;
		std::optional<Rect4f> textClip;
		
		AutoCompleteHandle autoCompleteHandle;
		StringUTF32 userInputForAutoComplete;
		Vector<StringUTF32> autoCompleteOptions;
		std::optional<size_t> autoCompleteCurOption;
		bool modifiedByAutoComplete = false;

		Vector<String> history;
		std::optional<size_t> historyCurOption;
		bool historyEnabled = false;
		bool modifiedByHistory = false;

		bool autoSizeHorizontal = true;
		std::optional<Range<float>> autoSizeRange;

		Vector2f textScrollPos;
		Vector2f caretPhysicalPos;
		float caretTime = 0;
		int caretPos = 0;
		bool caretShowing = false;
		bool mouseHeld = false;

		bool multiLine = false;
		bool clearOnSubmit = false;
		bool showGhostWhenFocused = false;
		bool selectAllOnClick = false;

		uint8_t clickTimeout = 0;
		Time timeSinceLastClick = 1;
		int consecutiveClickCount = 0;
	};
}
