#pragma once
#include "halley/text/halleystring.h"
#include <functional>

namespace Halley {
	class UIValidator {
	public:
		virtual ~UIValidator();

		virtual StringUTF32 onTextChanged(const StringUTF32& changedTo);
		virtual bool isEnabled();
	};

	class UINumericValidator : public UIValidator {
	public:
		explicit UINumericValidator(bool allowNegative);
		StringUTF32 onTextChanged(const StringUTF32& changedTo) override;

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
}
