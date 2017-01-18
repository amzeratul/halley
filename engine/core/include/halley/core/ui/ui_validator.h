#pragma once
#include "halley/text/halleystring.h"

namespace Halley {
	class UIValidator {
	public:
		virtual ~UIValidator();

		virtual StringUTF32 onTextChanged(const StringUTF32& changedTo);
	};

	class UINumericValidator : public UIValidator {
	public:
		explicit UINumericValidator(bool allowNegative);
		StringUTF32 onTextChanged(const StringUTF32& changedTo) override;

	private:
		bool allowNegative;
	};
}
