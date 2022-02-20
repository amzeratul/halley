#include "ui_validator.h"

using namespace Halley;

UIValidator::~UIValidator()
{
}

StringUTF32 UIValidator::onTextChanged(StringUTF32 changedTo)
{
	return changedTo;
}

bool UIValidator::isEnabled()
{
	return true;
}

UINumericValidator::UINumericValidator(bool allowNegative, bool allowFloat)
	: allowNegative(allowNegative)
	, allowFloat(allowFloat)
{
}

StringUTF32 UINumericValidator::onTextChanged(StringUTF32 src)
{
	Vector<utf32type> result(src.length());
	size_t j = 0;
	for (size_t i = 0; i < result.size(); ++i) {
		if ((src[i] >= '0' && src[i] <= '9') || (i == 0 && src[i] == '-' && allowNegative) || (src[i] == '.' && allowFloat)) {
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

UITextFunctionValidator::UITextFunctionValidator(std::function<StringUTF32(StringUTF32)> validate)
	: f(std::move(validate))
{
}

StringUTF32 UITextFunctionValidator::onTextChanged(StringUTF32 changedTo)
{
	return f(std::move(changedTo));
}
