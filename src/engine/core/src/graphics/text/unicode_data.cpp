#include "halley/graphics/text/unicode_data.h"

using namespace Halley;

UnicodeData::UnicodeData()
{
	loadLineBreakRules();
}

UnicodeData::LineBreakClass UnicodeData::getLineBreakClass(char32_t code) const
{
	if (code < lineBreakClassesAscii.size()) {
		return lineBreakClassesAscii[code];
	}

	const auto iter = lineBreakClasses.find(code);
	if (iter != lineBreakClasses.end()) {
		return iter->second;
	}
	
	// We check this AFTER the class check above, since they overlap, but line break classes must take priority
	// (otherwise things like east asian punctuation will count as ideographic class)
	if (isEastAsianIdeographicCharacter(code)) {
		return LineBreakClass::Ideographic;
	}

	return LineBreakClass::Alphabetic;
}

const UnicodeData::LineBreakRules& UnicodeData::getLineBreakRules(char32_t code, Context context) const
{
	return getLineBreakRules(getLineBreakClass(code), context);
}

const UnicodeData::LineBreakRules& UnicodeData::getLineBreakRules(LineBreakClass characterClass, Context context) const
{
	if (characterClass == LineBreakClass::InfixNumericSeparator && context == Context::Text) {
		characterClass = LineBreakClass::ClosePunctuation;
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

void UnicodeData::setClass(char32_t c, LineBreakClass breakClass)
{
	if (c < lineBreakClassesAscii.size()) {
		lineBreakClassesAscii[c] = breakClass;
	} else {
		lineBreakClasses[c] = breakClass;
	}
}

void UnicodeData::loadLineBreakRules()
{
	lineBreakClassesAscii.fill(LineBreakClass::Alphabetic);
	for (char32_t c = '0'; c <= '9'; ++c) {
		setClass(c, LineBreakClass::Numeric);
	}

	for (char32_t c: { U'\r' }) {
		setClass(c, LineBreakClass::CarriageReturn);
	}
	for (char32_t c: { U'\n' }) {
		setClass(c, LineBreakClass::LineFeed);
	}
	for (char32_t c: { U' ', U'\t' }) {
		setClass(c, LineBreakClass::Space);
	}
	for (char32_t c: { U',', U'.', U';', U':' }) {
		setClass(c, LineBreakClass::InfixNumericSeparator);
	}
	for (char32_t c: { U'-', U'!', U'?', U')', U']', U'}', U'、', U'。', U'…', U'‥', U'）', U'｝', U'］', U'】', U'」', U'』', U'⟩', U'〜', U'！', U'？' }) {
		setClass(c, LineBreakClass::ClosePunctuation);
	}
	for (char32_t c: { U'(', U'[', U'{', U'（', U'｛', U'［', U'【', U'「', U'『', U'⟨' }) {
		setClass(c, LineBreakClass::OpenPunctuation);
	}
	for (char32_t c: { U'%' }) {
		setClass(c, LineBreakClass::PostfixNumeric);
	}
	for (char32_t c: { U'$', U'£', U'€', U'¥' }) {
		setClass(c, LineBreakClass::PrefixNumeric);
	}
	for (char32_t c: { U'ぁ', U'ぃ', U'ぅ', U'ぇ', U'ぉ', U'っ', U'ゃ', U'ゅ', U'ょ', U'ゎ', U'ゕ',
		               U'ァ', U'ィ', U'ゥ', U'ェ', U'ォ', U'ッ', U'ャ', U'ュ', U'ョ', U'ヮ', U'ヵ', U'ー'}) {
		setClass(c, LineBreakClass::ConditionalJaStarter);
	}
	for (char32_t c: { U'々', U'〜', U'・'}) {
		setClass(c, LineBreakClass::NonStarter);
	}

	lineBreakRules[static_cast<int>(LineBreakClass::Break)]                 = LineBreakRules{ LineBreakType::Opportunity, LineBreakType::Force,       true,  false, Context::Undefined };
	lineBreakRules[static_cast<int>(LineBreakClass::CarriageReturn)]        = LineBreakRules{ LineBreakType::Neutral,     LineBreakType::Neutral,     true,  false, Context::Undefined };
	lineBreakRules[static_cast<int>(LineBreakClass::LineFeed)]              = LineBreakRules{ LineBreakType::Opportunity, LineBreakType::Force,       true,  false, Context::Undefined };
	lineBreakRules[static_cast<int>(LineBreakClass::Space)]                 = LineBreakRules{ LineBreakType::Opportunity, LineBreakType::Opportunity, true,  true,  Context::Undefined };
	lineBreakRules[static_cast<int>(LineBreakClass::Alphabetic)]            = LineBreakRules{ LineBreakType::Neutral,     LineBreakType::Neutral,     false, false, Context::Text };
	lineBreakRules[static_cast<int>(LineBreakClass::InfixNumericSeparator)] = LineBreakRules{ LineBreakType::Neutral,     LineBreakType::Neutral,     false, false, Context::Undefined };
	lineBreakRules[static_cast<int>(LineBreakClass::ClosePunctuation)]      = LineBreakRules{ LineBreakType::Prohibit,    LineBreakType::Opportunity, false, false, Context::Undefined };
	lineBreakRules[static_cast<int>(LineBreakClass::OpenPunctuation)]       = LineBreakRules{ LineBreakType::Opportunity, LineBreakType::Prohibit,    false, false, Context::Undefined };
	lineBreakRules[static_cast<int>(LineBreakClass::Ideographic)]           = LineBreakRules{ LineBreakType::OpportunityLowPriority, LineBreakType::OpportunityLowPriority, false, false, Context::Text };
	lineBreakRules[static_cast<int>(LineBreakClass::ConditionalJaStarter)]  = LineBreakRules{ LineBreakType::ProhibitUnlessAfterSpace,    LineBreakType::OpportunityLowPriority, false, false, Context::Text };
	lineBreakRules[static_cast<int>(LineBreakClass::NonStarter)]            = LineBreakRules{ LineBreakType::ProhibitUnlessAfterSpace,    LineBreakType::OpportunityLowPriority, false, false, Context::Text };
	lineBreakRules[static_cast<int>(LineBreakClass::EmojiBase)]             = LineBreakRules{ LineBreakType::Opportunity, LineBreakType::Opportunity, false, false, Context::Text };
	lineBreakRules[static_cast<int>(LineBreakClass::Numeric)]               = LineBreakRules{ LineBreakType::Neutral,     LineBreakType::Neutral,     false, false, Context::Numeric };
	lineBreakRules[static_cast<int>(LineBreakClass::PostfixNumeric)]        = LineBreakRules{ LineBreakType::Prohibit,    LineBreakType::Opportunity, false, false, Context::Undefined };
	lineBreakRules[static_cast<int>(LineBreakClass::PrefixNumeric)]         = LineBreakRules{ LineBreakType::Opportunity, LineBreakType::Prohibit,    false, false, Context::Undefined };
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

	if (aRules.after == UnicodeData::LineBreakType::Force) {
		return { 3, {}, true, true, false };
	} else if (aRules.isSpace) {
		return { 2, 1, false, true, aRules.consumeMoreSpace && aClass == bClass };
	} else if (aRules.after == UnicodeData::LineBreakType::Prohibit
			|| bRules.before == UnicodeData::LineBreakType::Prohibit
			|| (bRules.before == UnicodeData::LineBreakType::ProhibitUnlessAfterSpace && !aRules.isSpace)) {
		return { 0, {}, false, false, false };
	} else if (aRules.after == UnicodeData::LineBreakType::Opportunity
			|| bRules.before == UnicodeData::LineBreakType::Opportunity) {
		return { 2, 1, false, false, false };
	} else if (aRules.after == UnicodeData::LineBreakType::OpportunityLowPriority
			|| bRules.before == UnicodeData::LineBreakType::OpportunityLowPriority) {
		return { 1, {}, false, false, false };
	} else {
		return { 0, {}, false, false, false };
	}
}
