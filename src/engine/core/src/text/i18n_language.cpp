#include "halley/text/i18n_language.h"
#include "halley/text/halleystring.h"

using namespace Halley;

I18NLanguage::I18NLanguage()
{
}

I18NLanguage::I18NLanguage(const String& code)
{
	if (code.contains("-")) {
		auto split = code.split('-');
		set(split.at(0), split.at(1));
	} else if (code.contains("_")) {
		auto split = code.split('_');
		set(split.at(0), split.at(1));
	} else {
		set(code, {});
	}
}

I18NLanguage::I18NLanguage(String languageCode, std::optional<String> countryCode)
{
	set(std::move(languageCode), std::move(countryCode));
}

I18NLanguage::I18NLanguage(const ConfigNode& node)
{
	*this = I18NLanguage(node.asString(""));
}

ConfigNode I18NLanguage::toConfigNode() const
{
	return ConfigNode(getISOCode());
}

void I18NLanguage::set(String languageCode, std::optional<String> countryCode)
{
	this->languageCode = std::move(languageCode);
	this->countryCode = std::move(countryCode);
}

const String& I18NLanguage::getLanguageCode() const
{
	return languageCode;
}

const std::optional<String>& I18NLanguage::getCountryCode() const
{
	return countryCode;
}

String I18NLanguage::getISOCode() const
{
	if (countryCode) {
		return languageCode + "-" + countryCode.value();
	} else {
		return languageCode;
	}
}

char I18NLanguage::getDecimalSeparator() const
{
	if (languageCode == "fr" || languageCode == "pt" || languageCode == "es" || languageCode == "it" || languageCode == "de") {
		return ',';
	}
	return '.';
}

I18NLanguageMatch I18NLanguage::getMatch(const I18NLanguage& other) const
{
	if (languageCode != other.languageCode) {
		return I18NLanguageMatch::None;
	}
	if (countryCode != other.countryCode) {
		return I18NLanguageMatch::Good;
	}
	return I18NLanguageMatch::Exact;
}

std::optional<I18NLanguage> I18NLanguage::getBestMatch(const Vector<I18NLanguage>& languages, const I18NLanguage& target, std::optional<I18NLanguage> fallback)
{
	I18NLanguageMatch bestMatch = I18NLanguageMatch::None;
	std::optional<I18NLanguage> result = fallback;
	for (const auto& l: languages) {
		auto m = l.getMatch(target);
		if (int(m) > int(bestMatch)) {
			bestMatch = m;
			result = l;
		}
	}
	return result;
}

bool I18NLanguage::operator==(const I18NLanguage& other) const
{
	return languageCode == other.languageCode && countryCode == other.countryCode;
}

bool I18NLanguage::operator!=(const I18NLanguage& other) const
{
	return languageCode != other.languageCode || countryCode != other.countryCode;
}

bool I18NLanguage::operator<(const I18NLanguage& other) const
{
	if (languageCode != other.languageCode) {
		return languageCode < other.languageCode;
	}
	if (countryCode == other.countryCode) {
		return false;
	}
	if (!countryCode) {
		return true;
	}
	if (!other.countryCode) {
		return false;
	}
	return countryCode.value() < other.countryCode.value();
}

void I18NLanguage::serialize(Serializer& s) const
{
	s << languageCode;
	s << countryCode;
}

void I18NLanguage::deserialize(Deserializer& s)
{
	s >> languageCode;
	s >> countryCode;
}
