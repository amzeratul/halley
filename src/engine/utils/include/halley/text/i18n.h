#pragma once

#include "halleystring.h"
#include <map>

namespace Halley {
	class ConfigFile;
	class I18N;

	class LocalisedString
	{
		friend class I18N;

	public:
		LocalisedString();

		static LocalisedString fromHardcodedString(const char* str);
		static LocalisedString fromHardcodedString(const String& str);
		static LocalisedString fromUserString(const String& str);
		static LocalisedString fromNumber(int number);

		const String& getString() const;

	private:
		explicit LocalisedString(const String& string);

		String string;
	};

	class I18N {
	public:
		I18N();

		void setCurrentLanguage(const String& code);
		void setDefaultLanguage(const String& code);
		void loadLanguage(const String& code, const ConfigFile& config);

		LocalisedString get(const String& key) const;
		const String& getCurrentLanguage() const;

	private:
		String currentLanguage;
		String defaultLanguage;
		std::map<String, std::map<String, String>> strings;

		LocalisedString missingStr;
	};
}
