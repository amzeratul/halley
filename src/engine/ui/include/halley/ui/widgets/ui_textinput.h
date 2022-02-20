#pragma once

#include "../ui_widget.h"
#include "halley/core/graphics/sprite/sprite.h"
#include "halley/core/graphics/text/text_renderer.h"
#include "halley/core/input/input_device.h"
#include "halley/text/i18n.h"
#include "halley/core/input/input_keyboard.h"
#include "halley/core/input/text_input_data.h"

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
		LocalisedString getGhostText() const;
		
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

		void submit();
		void setClearOnSubmit(bool enabled);
		bool isClearOnSubmit() const;

		void setIcon(Sprite icon, Vector4f border);

		void setAutoSize(std::optional<Range<float>> range);

	protected:
		void draw(UIPainter& painter) const override;
		void update(Time t, bool moved) override;

		void onFocus() override;
		TextInputData* getTextInputData() override;

		bool onKeyPress(KeyboardKeyPress key) override;
		
		void pressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;

		void readFromDataBind() override;

		virtual Vector4f getTextInnerBorder() const;

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

		Sprite sprite;
		Sprite caret;
		Sprite icon;
		TextRenderer label;
		TextRenderer ghostLabel;
		Vector4f iconBorder;

		TextInputData text;
		LocalisedString ghostText;
		StringUTF32 lastText;
		
		AutoCompleteHandle autoCompleteHandle;
		StringUTF32 userInputForAutoComplete;
		Vector<StringUTF32> autoCompleteOptions;
		std::optional<size_t> autoCompleteCurOption;
		bool modifiedByAutoComplete = false;

		Vector<String> history;
		std::optional<size_t> historyCurOption;
		bool historyEnabled = false;
		bool modifiedByHistory = false;

		std::optional<Range<float>> autoSizeRange;

		Vector2f textScrollPos;
		float caretPhysicalPos = 0;
		float caretTime = 0;
		int caretPos = 0;
		bool caretShowing = false;

		bool isMultiLine = false;
		bool clearOnSubmit = false;
	};
}
