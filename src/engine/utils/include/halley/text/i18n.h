#pragma once

#include "halleystring.h"
#include <map>

namespace Halley {
	class ConfigNode;
	class ConfigFile;
	class ConfigObserver;
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

		LocalisedString replaceTokens(const LocalisedString& tok0) const;
		LocalisedString replaceTokens(const LocalisedString& tok0, const LocalisedString& tok1) const;

		const String& getString() const;

		bool operator==(const LocalisedString& other) const;
		bool operator!=(const LocalisedString& other) const;
		bool operator<(const LocalisedString& other) const;

		bool checkForUpdates();

	private:
		explicit LocalisedString(String string);
		explicit LocalisedString(const I18N& i18n, String key, String string);

		const I18N* i18n = nullptr;
		String key;
		String string;
		int i18nVersion = 0;
	};

	class I18N {
	public:
		I18N();

		void update();
		void loadLocalisationFile(const ConfigFile& config);

		void setCurrentLanguage(const String& code);
		void setFallbackLanguage(const String& code);
		std::vector<String> getLanguagesAvailable() const;

		LocalisedString get(const String& key) const;

		template <typename T>
		std::vector<LocalisedString> getVector(const String& keyPrefix, const T& keys) const
		{
			std::vector<LocalisedString> result;
			for (auto& k: keys) {
				result.push_back(get(keyPrefix + k));
			}
			return result;
		}

		const String& getCurrentLanguage() const;
		int getVersion() const;

	private:
		String currentLanguage;
		String fallbackLanguage;
		std::map<String, std::map<String, String>> strings;
		std::map<String, ConfigObserver> observers;
		int version = 0;

		LocalisedString missingStr;

		void loadLocalisation(const ConfigNode& node);
	};
}

