#include "halley/text/localised_string.h"

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
	: i18n(i18n)
	, string(std::move(string))
{
}

LocalisedString::LocalisedString(const I18N& i18n, String key, String string, int languageIndex)
	: i18n(&i18n)
	, key(std::move(key))
	, string(std::move(string))
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

LocalisedString LocalisedString::replaceTokens(gsl::span<const LocalisedString> toks) const
{
	Vector<const LocalisedString*> result;
	result.reserve(toks.size());
	for (const auto& t: toks) {
		result += &t;
	}
	return doReplaceTokens(result.const_span());
}

LocalisedString LocalisedString::doReplaceTokens(gsl::span<const LocalisedString* const> toks) const
{
	if (toks.empty()) {
		return *this;
	}
	auto str = string;
	for (int i = 0; i < int(toks.size()); ++i) {
		str = str.replaceAll("{" + Halley::toString(i) + "}", toks[i]->getString());
	}
	return LocalisedString(str, i18n);
}

std::pair<LocalisedString, Vector<ColourOverride>> LocalisedString::replaceTokens(gsl::span<const LocalisedString> toks, gsl::span<const std::optional<Colour4f>> colours) const
{
	HalleyAssertDev(toks.size() == colours.size());
	if (toks.empty()) {
		return { *this, {} };
	}

	Vector<std::pair<int, size_t>> indices;

	for (int i = 0; i < int(toks.size()); ++i) {
		const auto pos = string.find("{" + Halley::toString(i) + "}");
		if (pos != String::npos) {
			indices.emplace_back(i, pos);
		}
	}

	std::stable_sort(indices.begin(), indices.end(), [] (const auto& a, const auto& b) { return a.second < b.second; });

	auto str = std::string_view(string);

	size_t lastPos = 0;
	ColourStringBuilder builder;
	for (const auto& index: indices) {
		builder.append(str.substr(lastPos, index.second - lastPos));
		builder.append(toks[index.first].getString(), colours[index.first]);
		lastPos = index.second + 3;
	}
	builder.append(str.substr(lastPos));

	auto result = builder.moveResults();
	return { LocalisedString(std::move(result.first), i18n), std::move(result.second) };
}

LocalisedString LocalisedString::replaceTokens(const std::map<String, LocalisedString>& tokens) const
{
	auto curString = string;
	for (const auto& token : tokens) {
		curString = string.replaceAll("{" + token.first + "}", token.second.getString());
	}
	return LocalisedString(curString, i18n);
}

LocalisedString LocalisedString::replaceToken(const String& pattern, const LocalisedString& token) const
{
	return LocalisedString(string.replaceAll(pattern, token.getString()), i18n);
}

LocalisedString LocalisedString::replaceLanguage(const I18NLanguage& language) const
{
	if (i18n) {
		return i18n->get(key, language);
	}
	return *this;
}

const String& LocalisedString::getString() const
{
	return string;
}

const String& LocalisedString::toString() const
{
	return string;
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
	return LocalisedString(string + other.string, i18n);
}

bool LocalisedString::checkForUpdates()
{
	if (i18n && !key.isEmpty()) {
		const auto curVersion = i18n->getVersion();
		if (i18nVersion != curVersion) {
			const auto newValue = i18n->get(key, getLanguage(*i18n));
			i18nVersion = curVersion;
			if (string != newValue.string) {
				string = newValue.string;
				return true;
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
