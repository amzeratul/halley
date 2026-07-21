#pragma once

#include "halley/maths/rect.h"
#include "halley/text/halleystring.h"

namespace Halley {
	class UIRootGroup;
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
		virtual void onEditingText(const StringUTF32& text);
		virtual bool onKeyPress(KeyboardKeyPress c, IClipboard* clipboard);

		virtual bool isActive() const;
		virtual void setUIRootGroup(UIRootGroup* rootGroup);
		
		virtual std::optional<Rect4f> getRect() const;
	};

	class StandardTextInputCapture final : public ITextInputCapture {
	public:
		StandardTextInputCapture(InputKeyboard& parent);
		~StandardTextInputCapture() override;

		void open(TextInputData& input, SoftwareKeyboardData softKeyboardData) override;
		void close() override;
		bool isOpen() const override;
		void update() override;

		void onTextEntered(const StringUTF32& text) override;
		void onEditingText(const StringUTF32& text) override;
		bool onKeyPress(KeyboardKeyPress c, IClipboard* clipboard) override;
		bool isActive() const override;

		void setUIRootGroup(UIRootGroup* rootGroup) override;
		
		std::optional<Rect4f> getRect() const override;

	private:
		bool currentlyOpen = false;
		InputKeyboard& parent;
		TextInputData* textInput;
		UIRootGroup* rootGroup = nullptr;
	};

	class HotkeyTextInputCapture : public ITextInputCapture {
	public:
		size_t addHotkey(KeyboardKeyPress c);
		bool isHotkeyPressed(size_t idx);

		void open(TextInputData& input, SoftwareKeyboardData softKeyboardData) override;
		void close() override;
		bool isOpen() const override;
		void update() override;

		bool onKeyPress(KeyboardKeyPress c, IClipboard* clipboard) override;

	private:
		Vector<KeyboardKeyPress> hotkeys;
		Vector<bool> pressed;
	};

	class TextInputCapture {
	public:
		TextInputCapture(TextInputData& inputData, SoftwareKeyboardData data, std::unique_ptr<ITextInputCapture> capture);
		~TextInputCapture();

		TextInputCapture(TextInputCapture&& other) noexcept = default;
		TextInputCapture(const TextInputCapture& other) = delete;
		TextInputCapture& operator=(TextInputCapture&& other) noexcept = default;
		TextInputCapture& operator=(const TextInputCapture& other) = delete;

		[[maybe_unused]] bool update() const; // Returns if the capture is still open

	private:
		std::unique_ptr<ITextInputCapture> capture;
	};
}
