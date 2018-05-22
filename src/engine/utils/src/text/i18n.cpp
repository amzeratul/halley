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
			return LocalisedString(i->second);
		}
	}

	if (!fallbackLanguage.isEmpty() && fallbackLanguage != currentLanguage) {
		auto defLang = strings.find(fallbackLanguage);
		if (defLang != strings.end()) {
			auto i = defLang->second.find(key);
			if (i != defLang->second.end()) {
				return LocalisedString(i->second);
			}
		}
	}

	return missingStr;
}

const String& I18N::getCurrentLanguage() const
{
	return currentLanguage;
}

LocalisedString::LocalisedString()
{
}

LocalisedString::LocalisedString(const String& string)
	: string(string)
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

LocalisedString LocalisedString::replaceTokens(const LocalisedString& tok) const
{
	return LocalisedString(string.replaceAll("{0}", tok.getString()));
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
