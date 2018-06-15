#pragma once
#include "halley/text/halleystring.h"
#include <functional>

namespace Halley {
	class UIValidator {
	public:
		virtual ~UIValidator();

		virtual StringUTF32 onTextChanged(StringUTF32 changedTo);
		virtual bool isEnabled();
	};

	class UINumericValidator : public UIValidator {
	public:
		explicit UINumericValidator(bool allowNegative);
		StringUTF32 onTextChanged(StringUTF32) override;

	private:
		bool allowNegative;
	};

	class UIFunctionValidator : public UIValidator {
	public:
		explicit UIFunctionValidator(std::function<bool()> validate);
		bool isEnabled() override;

	private:
		std::function<bool()> f;
	};

	class UITextFunctionValidator : public UIValidator {
	public:
		explicit UITextFunctionValidator(std::function<StringUTF32(StringUTF32)> validate);
		StringUTF32 onTextChanged(StringUTF32 changedTo) override;

	private:
		std::function<StringUTF32(StringUTF32)> f;
	};
}
