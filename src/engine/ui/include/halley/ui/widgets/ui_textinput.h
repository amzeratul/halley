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
		explicit UITextInput(std::shared_ptr<InputKeyboard> keyboard, String id, UIStyle style, String text = "", LocalisedString ghostText = LocalisedString());

		UITextInput(UITextInput&& other) = delete;
		UITextInput(const UITextInput& other) = delete;
		UITextInput& operator=(UITextInput&& other) = delete;
		UITextInput& operator=(const UITextInput& other) = delete;

		bool canInteractWithMouse() const override;

		UITextInput& setText(const String& text);
		UITextInput& setText(StringUTF32 text);

		String getText() const;
		UITextInput& setGhostText(const LocalisedString& text);
		LocalisedString getGhostText() const;

		Maybe<int> getMaxLength() const;
		void setMaxLength(Maybe<int> length);

		void onManualControlActivate() override;

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

		void onTextModified();
		void validateText();
		void onValidatorSet() override;

		std::shared_ptr<InputKeyboard> keyboard;
		std::unique_ptr<TextInputCapture> capture;

		UIStyle style;
		Sprite sprite;
		Sprite caret;
		TextRenderer label;

		TextInputData text;
		LocalisedString ghostText;

		Vector2f textScrollPos;
		float caretPhysicalPos = 0;
		float caretTime = 0;
		int caretPos = 0;

		bool isMultiLine = false;
		bool caretShowing = false;
	};
}
