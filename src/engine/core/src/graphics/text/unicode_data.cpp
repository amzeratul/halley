#include "halley/graphics/text/unicode_data.h"

using namespace Halley;

UnicodeData::UnicodeData()
{
	loadLineBreakRules();
}

UnicodeData::LineBreakClass UnicodeData::getLineBreakClass(char32_t code) const
{
	const auto iter = lineBreakClasses.find(code);
	if (iter != lineBreakClasses.end()) {
		return iter->second;
	}

	if (isEastAsianIdeographicCharacter(code)) {
		return LineBreakClass::ID;
	}

	return LineBreakClass::AL;
}

const UnicodeData::LineBreakRules& UnicodeData::getLineBreakRules(char32_t code, Context context) const
{
	return getLineBreakRules(getLineBreakClass(code), context);
}

const UnicodeData::LineBreakRules& UnicodeData::getLineBreakRules(LineBreakClass characterClass, Context context) const
{
	if (characterClass == LineBreakClass::IS && context == Context::Text) {
		characterClass = LineBreakClass::CL;
	}
	return lineBreakRules[static_cast<int>(characterClass)];
}

UnicodeData::Context UnicodeData::getNewContext(LineBreakClass charClass) const
{
	return getLineBreakRules(charClass).context;
}

UnicodeData::Context UnicodeData::getNewContext(LineBreakClass curCharClass, LineBreakClass nextCharClass) const
{
	if (auto aTarget = getNewContext(curCharClass); aTarget != Context::Undefined) {
		return aTarget;
	} else {
		return getNewContext(nextCharClass);
	}
}

void UnicodeData::loadLineBreakRules()
{
	for (char32_t c : { U'\n' }) {
		lineBreakClasses[c] = LineBreakClass::BK;
	}
	for (char32_t c : { U' ', U'\t' }) {
		lineBreakClasses[c] = LineBreakClass::SP;
	}
	for (char32_t c : { U',', U'.', U';', U':' }) {
		lineBreakClasses[c] = LineBreakClass::IS;
	}
	for (char32_t c : { U'-', U'!', U'?', U')', U']', U'}', U'、', U'。', U'…', U'‥', U'）', U'｝', U'］', U'】', U'」', U'』', U'⟩', U'〜', U'！', U'？' }) {
		lineBreakClasses[c] = LineBreakClass::CL;
	}
	for (char32_t c : { U'(', U'[', U'{', U'（', U'｛', U'［', U'【', U'「', U'『', U'⟨' }) {
		lineBreakClasses[c] = LineBreakClass::OP;
	}
	for (char32_t c : { U'0', U'1', U'2', U'3', U'4', U'5', U'6', U'7', U'8', U'9' }) {
		lineBreakClasses[c] = LineBreakClass::NU;
	}
	for (char32_t c : { U'%' }) {
		lineBreakClasses[c] = LineBreakClass::PO;
	}
	for (char32_t c : { U'$', U'£', U'€', U'¥' }) {
		lineBreakClasses[c] = LineBreakClass::PR;
	}

	lineBreakRules[static_cast<int>(LineBreakClass::BK)] = LineBreakRules{ true, true, false, false, true, false, Context::Undefined };
	lineBreakRules[static_cast<int>(LineBreakClass::SP)] = LineBreakRules{ true, true, false, false, true, true, Context::Undefined };
	lineBreakRules[static_cast<int>(LineBreakClass::AL)] = LineBreakRules{ false, false, false, false, false, false, Context::Text };
	lineBreakRules[static_cast<int>(LineBreakClass::IS)] = LineBreakRules{ false, false, false, false, false, false, Context::Undefined };
	lineBreakRules[static_cast<int>(LineBreakClass::CL)] = LineBreakRules{ false, true, true, false, false, false, Context::Undefined };
	lineBreakRules[static_cast<int>(LineBreakClass::OP)] = LineBreakRules{ true, false, false, true, false, false, Context::Undefined };
	lineBreakRules[static_cast<int>(LineBreakClass::ID)] = LineBreakRules{ true, true, false, false, false, false, Context::Text };
	lineBreakRules[static_cast<int>(LineBreakClass::EB)] = LineBreakRules{ true, true, false, false, false, false, Context::Text };
	lineBreakRules[static_cast<int>(LineBreakClass::NU)] = LineBreakRules{ false, false, false, false, false, false, Context::Numeric };
	lineBreakRules[static_cast<int>(LineBreakClass::PO)] = LineBreakRules{ false, true, true, false, false, false, Context::Undefined };
	lineBreakRules[static_cast<int>(LineBreakClass::PR)] = LineBreakRules{ true, false, false, true, false, false, Context::Undefined };
}

bool UnicodeData::isEastAsianIdeographicCharacter(char32_t c)
{
	return
		(c >= 0x3000 && c <= 0x30FF) // Punctuation and Kana
		|| (c >= 0x4e00 && c <= 0x9FAF) // CJK ideograms
		|| (c >= 0xAC00 && c <= 0xD7AF) // Hangul
		|| (c >= 0xFF00 && c <= 0xFFEF) // Halfwidth
		|| (c >= 0x1B000 && c <= 0x1B16F); // Kana supplement & extended
}

UnicodeLineBreaker::UnicodeLineBreaker(const UnicodeData& data)
	: data(data)
{
}

UnicodeLineBreaker::Result UnicodeLineBreaker::feedCharacter(char32_t a, char32_t b)
{
	const auto aClass = data.getLineBreakClass(a);
	const auto bClass = data.getLineBreakClass(b);

	if (auto newContext = data.getNewContext(aClass, bClass); newContext != UnicodeData::Context::Undefined) {
		context = newContext;
	}

	const auto& aRules = data.getLineBreakRules(aClass, context);
	const auto& bRules = data.getLineBreakRules(bClass, context);

	if (aRules.isSpace) {
		return { 1, true, aRules.consumeMoreSpace && aClass == bClass };
	} else if (aRules.prohibitLineBreakAfter || bRules.prohibitLineBreakBefore) {
		return { 0, false, false };
	} else if (aRules.lineBreakOpportunityAfter || bRules.lineBreakOpportunityBefore) {
		return { 1, false, false };
	} else {
		return { 0, false, false };
	}
}
