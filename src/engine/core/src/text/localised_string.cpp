#include "halley/text/localised_string.h"

#include "localised_string_transform_operation.h"
#include "halley/text/i18n.h"

using namespace Halley;

LocalisedString::LocalisedString()
{
}

LocalisedString& LocalisedString::operator+=(const LocalisedString& str)
{
	string += str.string;
	return *this;
}

LocalisedString::LocalisedString(String string, const I18N* i18n)
	: string(std::move(string))
	, i18n(i18n)
{
}

LocalisedString::LocalisedString(const I18N& i18n, String key, String string, int languageIndex)
	: string(std::move(string))
	, key(std::move(key))
	, i18n(&i18n)
	, i18nVersion(i18n.getVersion())
	, languageIdx(languageIndex)
{
}

LocalisedString LocalisedString::fromHardcodedString(const char* str)
{
	return LocalisedString(String(str), nullptr);
}

LocalisedString LocalisedString::fromHardcodedString(const String& str)
{
	return LocalisedString(String(str), nullptr);
}

LocalisedString LocalisedString::fromUserString(const String& str)
{
	return LocalisedString(str, nullptr);
}

LocalisedString LocalisedString::fromNumber(int number, int base, int width, char fill)
{
	return LocalisedString(Halley::toString(number, base, width, fill), nullptr);
}

LocalisedString LocalisedString::fromNumber(float number, const I18NLanguage& language, int precisionDigits, bool fixed)
{
	return LocalisedString(Halley::toString(number, precisionDigits, language.getDecimalSeparator(), fixed), nullptr);
}

std::pair<LocalisedString, Vector<ColourOverride>> LocalisedString::replaceTokens(gsl::span<const LocalisedString> toks, gsl::span<const std::optional<Colour4f>> colours) const
{
	auto str = doReplaceTokens(Vector<LocalisedString>(toks.begin(), toks.end()));
	auto cols = str.makeColourOverrides(colours);
	return { str, cols };
}

LocalisedString LocalisedString::replaceTokens(gsl::span<const LocalisedString> toks) const
{
	return doReplaceTokens(Vector<LocalisedString>(toks.begin(), toks.end()));
}

LocalisedString LocalisedString::replaceTokens(const std::map<String, LocalisedString>& tokens) const
{
	Vector<String> ids;
	Vector<LocalisedString> ts;
	for (const auto& [k, v] : tokens) {
		ids += k;
		ts += v;
	}
	return doReplaceTokens(std::move(ids), std::move(ts));
}

LocalisedString LocalisedString::replaceToken(const String& pattern, const LocalisedString& token) const
{
	Vector<String> ids;
	Vector<LocalisedString> ts;
	ids += pattern;
	ts += token;
	return doReplaceTokens(std::move(ids), std::move(ts));
}

LocalisedString LocalisedString::doReplaceTokens(Vector<LocalisedString> toks) const
{
	Vector<String> ids;
	ids.reserve(toks.size());
	for (size_t i = 0; i < toks.size(); ++i) {
		ids += Halley::toString(i);
	}

	return doReplaceTokens(std::move(ids), std::move(toks));
}

LocalisedString LocalisedString::doReplaceTokens(Vector<String> ids, Vector<LocalisedString> toks) const
{
	auto op = std::make_shared<LocStrOpReplaceTokens>(*this, std::move(ids), std::move(toks));
	LocalisedString result = *this;
	result.transformOp = std::move(op);
	result.applyTransformOperation();
	return result;
}

void LocalisedString::applyTransformOperation()
{
	if (transformOp) {
		transformOp->eval(*this);
	}
}

LocalisedString LocalisedString::replaceLanguage(const I18NLanguage& language) const
{
	auto result = *this;
	if (result.transformOp) {
		result.transformOp = result.transformOp->clone();
	}
	result.replaceLanguageInPlace(language);
	return result;
}

void LocalisedString::replaceLanguageInPlace(const I18NLanguage& language)
{
	if (transformOp) {
		transformOp->setLanguage(language);
		applyTransformOperation();
	} else if (i18n) {
		*this = i18n->get(key, language);
	}
}

const String& LocalisedString::getString() const
{
	return string;
}

const String& LocalisedString::toString() const
{
	return string;
}

bool LocalisedString::isSameKeyAndTransform(const LocalisedString& other) const
{
	if (key != other.key) {
		return false;
	}

	if (transformOp && other.transformOp) {
		return transformOp->isEquivalentTo(*other.transformOp);
	} else {
		// Equivalent if neither has a transform; otherwise one has it and the other doesn't
		return !transformOp && !other.transformOp;
	}
}

bool LocalisedString::operator==(const LocalisedString& other) const
{
	return string == other.string;
}

bool LocalisedString::operator!=(const LocalisedString& other) const
{
	return string != other.string;
}

bool LocalisedString::operator<(const LocalisedString& other) const
{
	return string < other.string;
}

LocalisedString LocalisedString::operator+(const LocalisedString& other) const
{
	auto result = LocalisedString(string + other.string, i18n);
	result.tokenInfo = tokenInfo + other.tokenInfo;
	return result;
}

bool LocalisedString::checkForUpdates()
{
	if (i18n && !key.isEmpty()) {
		const auto curVersion = i18n->getVersion();
		if (i18nVersion != curVersion) {
			const auto oldVersion = i18nVersion;
			i18nVersion = curVersion;

			if (transformOp) {
				if (transformOp->checkForUpdates()) {
					applyTransformOperation();
					return true;
				}
			} else {
				auto newValue = i18n->get(key);
				if (newValue.string != string || newValue.languageIdx != languageIdx) {
					*this = std::move(newValue);
					return true;
				}
			}
		}
	}
	return false;
}

const String& LocalisedString::getKey() const
{
	return key;
}

const I18NLanguage* LocalisedString::tryGetLanguage() const
{
	return i18n ? &i18n->getLanguageFromIndex(languageIdx) : nullptr;
}

const I18NLanguage& LocalisedString::getLanguage(const I18N& i18n) const
{
	return i18n.getLanguageFromIndex(languageIdx);
}

const Vector<LocalisedString::TokenInfo>& LocalisedString::getTokenInfo() const
{
	return tokenInfo;
}

Vector<ColourOverride> LocalisedString::makeColourOverrides(gsl::span<const std::optional<Colour4f>> colours) const
{
	// TODO: could remove redundant transitions if two tokens are adjacent

	Vector<ColourOverride> result;
	result.reserve(tokenInfo.size() * 2);
	for (const auto& token: tokenInfo) {
		if (token.idx < colours.size()) {
			result += ColourOverride(token.pos, colours[token.idx]);
			result += ColourOverride(token.pos + token.len, std::nullopt);
		}
	}
	return result;
}
