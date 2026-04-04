#pragma once
#include "halley/text/halleystring.h"
#include <functional>

namespace Halley {
	class UIValidator {
	public:
		virtual ~UIValidator();

		virtual std::optional<StringUTF32> onTextChanged(StringUTF32 changedTo);
		virtual std::optional<bool> isEnabled();
	};

	class UINumericValidator final : public UIValidator {
	public:
		UINumericValidator(bool allowNegative, bool allowFloatingPoint = false);
		std::optional<StringUTF32> onTextChanged(StringUTF32) override;

	private:
		bool allowNegative;
		bool allowFloat;
	};

	class UIFunctionValidator final : public UIValidator {
	public:
		explicit UIFunctionValidator(std::function<bool()> validate);
		std::optional<bool> isEnabled() override;

	private:
		std::function<bool()> f;
	};

	class UITextFunctionValidator final : public UIValidator {
	public:
		explicit UITextFunctionValidator(std::function<StringUTF32(StringUTF32)> validate);
		std::optional<StringUTF32> onTextChanged(StringUTF32 changedTo) override;

	private:
		std::function<StringUTF32(StringUTF32)> f;
	};
}
