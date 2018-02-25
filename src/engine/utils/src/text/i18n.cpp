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

void I18N::setDefaultLanguage(const String& code)
{
	defaultLanguage = code;
}

void I18N::loadLanguage(const String& code, const ConfigFile& config)
{
	std::map<String, String> result;
	for (auto e: config.getRoot().asMap()) {
		result[e.first] = e.second.asString();
	}

	strings[code] = std::move(result);
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

	auto defLang = strings.find(defaultLanguage);
	if (defLang != strings.end()) {
		auto i = defLang->second.find(key);
		if (i != defLang->second.end()) {
			return LocalisedString(i->second);
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

const String& LocalisedString::getString() const
{
	return string;
}
