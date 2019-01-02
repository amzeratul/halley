#include <utility>
#include "halley/text/i18n.h"
#include "halley/file_formats/config_file.h"

using namespace Halley;

I18N::I18N()
{
}

void I18N::update()
{
	for (auto& o: observers) {
		if (o.second.needsUpdate()) {
			o.second.update();
			loadLocalisation(o.second.getRoot());
		}
	}
}

void I18N::setCurrentLanguage(const I18NLanguage& code)
{
	currentLanguage = code;
	++version;
}

void I18N::setFallbackLanguage(const I18NLanguage& code)
{
	fallbackLanguage = code;
}

void I18N::loadLocalisationFile(const ConfigFile& config)
{
	loadLocalisation(config.getRoot());
	observers[config.getAssetId()] = ConfigObserver(config);
}

void I18N::loadLocalisation(const ConfigNode& root)
{
	for (auto& language: root.asMap()) {
		auto& langCode = language.first;
		auto& lang = strings[langCode];
		for (auto& e: language.second.asMap()) {
			lang[e.first] = e.second.asString();
		}
	}
	++version;
}

std::vector<I18NLanguage> I18N::getLanguagesAvailable() const
{
	std::vector<I18NLanguage> result;
	for (auto& e: strings) {
		result.push_back(e.first);
	}
	return result;
}

LocalisedString I18N::get(const String& key) const
{
	auto curLang = strings.find(currentLanguage);
	if (curLang != strings.end()) {
		auto i = curLang->second.find(key);
		if (i != curLang->second.end()) {
			return LocalisedString(*this, key, i->second);
		}
	}

	if (fallbackLanguage && fallbackLanguage.get() != currentLanguage) {
		auto defLang = strings.find(fallbackLanguage.get());
		if (defLang != strings.end()) {
			auto i = defLang->second.find(key);
			if (i != defLang->second.end()) {
				return LocalisedString(*this, key, i->second);
			}
		}
	}

	return LocalisedString(*this, key, "#MISSING#");
}

LocalisedString I18N::getPreProcessedUserString(const String& string) const
{
	if (string.startsWith("$")) {
		return get(string.mid(1));
	} else {
		return LocalisedString::fromUserString(string);
	}
}

const I18NLanguage& I18N::getCurrentLanguage() const
{
	return currentLanguage;
}

int I18N::getVersion() const
{
	return version;
}

char I18N::getDecimalSeparator() const
{
	const auto& lang = currentLanguage.getLanguageCode();
	if (lang == "fr" || lang == "pt" || lang == "es" || lang == "it") {
		return ',';
	}
	return '.';
}

LocalisedString::LocalisedString()
{
}

LocalisedString& LocalisedString::operator+=(const LocalisedString& str)
{
	string += str.string;
	return *this;
}

LocalisedString::LocalisedString(String string)
	: string(std::move(string))
{
}

LocalisedString::LocalisedString(const I18N& i18n, String key, String string)
	: i18n(&i18n)
	, key(std::move(key))
	, string(std::move(string))
	, i18nVersion(i18n.getVersion())
{
}

I18NLanguage::I18NLanguage()
{
}

I18NLanguage::I18NLanguage(const String& code)
{
	if (code.contains("-")) {
		auto split = code.split('-');
		languageCode = split.at(0).asciiLower();
		countryCode = split.at(1).asciiLower();
	} else if (code.contains("_")) {
		auto split = code.split('_');
		languageCode = split.at(0).asciiLower();
		countryCode = split.at(1).asciiLower();
	} else {
		languageCode = code;
		countryCode = {};
	}
}

I18NLanguage::I18NLanguage(String languageCode, Maybe<String> countryCode)
	: languageCode(std::move(languageCode))
	, countryCode(std::move(countryCode))
{
}

const String& I18NLanguage::getLanguageCode() const
{
	return languageCode;
}

const Maybe<String>& I18NLanguage::getCountryCode() const
{
	return countryCode;
}

String I18NLanguage::getISOCode() const
{
	if (countryCode) {
		return languageCode + "-" + countryCode.get().asciiUpper();
	} else {
		return languageCode;
	}
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
	return countryCode.get() < other.countryCode.get();
}

LocalisedString LocalisedString::fromHardcodedString(const char* str)
{
	return LocalisedString(String(str));
}

LocalisedString LocalisedString::fromHardcodedString(const String& str)
{
	return LocalisedString(String(str));
}

LocalisedString LocalisedString::fromUserString(const String& str)
{
	return LocalisedString(str);
}

LocalisedString LocalisedString::fromNumber(int number)
{
	return LocalisedString(toString(number));
}

LocalisedString LocalisedString::replaceTokens(const LocalisedString& tok0) const
{
	return LocalisedString(string.replaceAll("{0}", tok0.getString()));
}

LocalisedString LocalisedString::replaceTokens(const LocalisedString& tok0, const LocalisedString& tok1) const
{
	return LocalisedString(string.replaceAll("{0}", tok0.getString()).replaceAll("{1}", tok1.getString()));
}

LocalisedString LocalisedString::replaceTokens(const LocalisedString& tok0, const LocalisedString& tok1, const LocalisedString& tok2) const
{
	return LocalisedString(string.replaceAll("{0}", tok0.getString()).replaceAll("{1}", tok1.getString()).replaceAll("{2}", tok2.getString()));
}

const String& LocalisedString::getString() const
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

bool LocalisedString::checkForUpdates()
{
	if (i18n) {
		const auto curVersion = i18n->getVersion();
		if (i18nVersion != curVersion) {
			const auto newValue = i18n->get(key);
			i18nVersion = curVersion;
			if (string != newValue.string) {
				string = newValue.string;
				return true;
			}
		}
	}
	return false;
}
