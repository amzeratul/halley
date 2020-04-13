#pragma once

#include "../ui_widget.h"
#include "halley/core/graphics/sprite/sprite.h"
#include "halley/core/graphics/text/text_renderer.h"
#include "halley/core/input/input_device.h"
#include "halley/text/i18n.h"
#include "halley/core/input/input_keyboard.h"

namespace Halley {
	class UIStyle;
	class IClipboard;

	class UITextInput : public UIWidget {
	public:
		using AutoCompleteHandle = std::function<std::vector<StringUTF32>(StringUTF32)>;
		
		explicit UITextInput(std::shared_ptr<InputKeyboard> keyboard, String id, UIStyle style, String text = "", LocalisedString ghostText = LocalisedString());

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

		std::optional<int> getMaxLength() const;
		void setMaxLength(std::optional<int> length);

		Range<int> getSelection() const;
		void setSelection(int selection);
		void setSelection(Range<int> selection);

		void onManualControlActivate() override;

		void setAutoCompleteHandle(AutoCompleteHandle handle);

		bool canReceiveFocus() const override;

		void setReadOnly(bool enabled);
		bool isReadOnly() const;

	protected:
		void draw(UIPainter& painter) const override;
		void update(Time t, bool moved) override;

		void onFocus() override;
		void onFocusLost() override;
		void pressMouse(Vector2f mousePos, int button) override;

		void readFromDataBind() override;

	private:
		void updateTextInput();
		void updateCaret();

		void onMaybeTextModified();
		void onTextModified();
		void validateText();
		void onValidatorSet() override;
		void updateAutoComplete();

		std::shared_ptr<InputKeyboard> keyboard;
		std::unique_ptr<TextInputCapture> capture;
		AutoCompleteHandle handle;

		UIStyle style;
		Sprite sprite;
		Sprite caret;
		TextRenderer label;
		TextRenderer ghostLabel;

		TextInputData text;
		LocalisedString ghostText;
		StringUTF32 lastText;
		
		StringUTF32 autoCompleteText;
		AutoCompleteHandle autoCompleteHandle;
		int autoCompleteOptions = 0;
		int autoCompleteCurOption = 0;

		Vector2f textScrollPos;
		float caretPhysicalPos = 0;
		float caretTime = 0;
		int caretPos = 0;

		bool isMultiLine = false;
		bool caretShowing = false;
	};
}
