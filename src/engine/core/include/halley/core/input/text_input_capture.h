#pragma once
#include "halley/text/halleystring.h"

namespace Halley {
	class TextInputData;

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

		TextInputCapture(TextInputCapture&& other) noexcept = default;
		TextInputCapture(const TextInputCapture& other) = delete;
		TextInputCapture& operator=(TextInputCapture&& other) noexcept = default;
		TextInputCapture& operator=(const TextInputCapture& other) = delete;

		bool update() const; // Returns if the capture is still open

	private:
		std::unique_ptr<ITextInputCapture> capture;
	};
}
