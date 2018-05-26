#include <utility>
#include "halley/text/i18n.h"
#include "halley/file_formats/config_file.h"

using namespace Halley;

I18N::I18N()
	: missingStr("#MISSING#")
{
}

void I18N::setCurrentLanguage(const String& code)
{
	currentLanguage = code;
	++version;
}

void I18N::setFallbackLanguage(const String& code)
{
	fallbackLanguage = code;
}

void I18N::loadLocalisationFile(const ConfigFile& config)
{
	for (auto& language: config.getRoot().asMap()) {
		auto& langCode = language.first;
		auto& lang = strings[langCode];
		for (auto& e: language.second.asMap()) {
			lang[e.first] = e.second.asString();
		}
	}
	++version;
}

std::vector<String> I18N::getLanguagesAvailable() const
{
	std::vector<String> result;
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

	if (!fallbackLanguage.isEmpty() && fallbackLanguage != currentLanguage) {
		auto defLang = strings.find(fallbackLanguage);
		if (defLang != strings.end()) {
			auto i = defLang->second.find(key);
			if (i != defLang->second.end()) {
				return LocalisedString(*this, key, i->second);
			}
		}
	}

	return missingStr;
}

const String& I18N::getCurrentLanguage() const
{
	return currentLanguage;
}

int I18N::getVersion() const
{
	return version;
}

LocalisedString::LocalisedString()
{
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
