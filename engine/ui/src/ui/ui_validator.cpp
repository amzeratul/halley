#include "ui_validator.h"

using namespace Halley;

UIValidator::~UIValidator()
{
}

StringUTF32 UIValidator::onTextChanged(const StringUTF32& changedTo)
{
	return changedTo;
}

bool UIValidator::isEnabled()
{
	return true;
}

UINumericValidator::UINumericValidator(bool allowNegative)
	: allowNegative(allowNegative)
{
}

StringUTF32 UINumericValidator::onTextChanged(const StringUTF32& src)
{
	std::vector<utf32type> result(src.length());
	size_t j = 0;
	for (size_t i = 0; i < result.size(); ++i) {
		if ((src[i] >= '0' && src[i] <= '9') || (src[i] == '-' && allowNegative)) {
			result[j++] = src[i];
		}
	}
	return StringUTF32(result.data(), j);
}

UIFunctionValidator::UIFunctionValidator(std::function<bool()> f)
	: f(f)
{
}

bool UIFunctionValidator::isEnabled()
{
	return f();
}
