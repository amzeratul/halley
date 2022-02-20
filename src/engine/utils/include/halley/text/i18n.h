#pragma once

#include "halleystring.h"
#include <map>

#include "halley/core/resources/resources.h"
#include "halley/data_structures/maybe.h"

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

		LocalisedString(const LocalisedString& other) = default;
		LocalisedString(LocalisedString&& other) noexcept = default;

		LocalisedString& operator=(const LocalisedString& other) = default;
		LocalisedString& operator=(LocalisedString&& other) noexcept = default;
		
		LocalisedString& operator+=(const LocalisedString& str);

		static LocalisedString fromHardcodedString(const char* str);
		static LocalisedString fromHardcodedString(const String& str);
		static LocalisedString fromUserString(const String& str);
		static LocalisedString fromNumber(int number);

		LocalisedString replaceTokens(const LocalisedString& tok0) const;
		LocalisedString replaceTokens(const LocalisedString& tok0, const LocalisedString& tok1) const;
		LocalisedString replaceTokens(const LocalisedString& tok0, const LocalisedString& tok1, const LocalisedString& tok2) const;
		LocalisedString replaceTokens(const std::map<String, LocalisedString>& tokens);

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

	enum class I18NLanguageMatch {
		None,
		Good,
		Exact
	};

	class I18NLanguage {
	public:
		I18NLanguage();
		explicit I18NLanguage(const String& code);
		I18NLanguage(String languageCode, std::optional<String> countryCode);

		void set(String languageCode, std::optional<String> countryCode);

		const String& getLanguageCode() const;
		const std::optional<String>& getCountryCode() const;
		String getISOCode() const;

		I18NLanguageMatch getMatch(const I18NLanguage& other) const;

		static std::optional<I18NLanguage> getBestMatch(const Vector<I18NLanguage>& languages, const I18NLanguage& target, std::optional<I18NLanguage> fallback = {});

		bool operator==(const I18NLanguage& other) const;
		bool operator!=(const I18NLanguage& other) const;
		bool operator<(const I18NLanguage& other) const;

	private:
		String languageCode;
		std::optional<String> countryCode;
	};

	class II18N {
	public:
		virtual ~II18N() = default;

		virtual LocalisedString get(const String& key) const = 0;
	};

	class I18N : public II18N {
	public:
		I18N();
		I18N(Resources& resources, I18NLanguage currentLanguage = I18NLanguage("en-GB"));

		void update();
		void loadStrings(Resources& resources);
		void loadLocalisationFile(const ConfigFile& config);

		void setCurrentLanguage(const I18NLanguage& code);
		void setFallbackLanguage(const I18NLanguage& code);
		Vector<I18NLanguage> getLanguagesAvailable() const;

		LocalisedString get(const String& key) const override;
		std::optional<LocalisedString> get(const String& key, const I18NLanguage& language) const;
		LocalisedString getPreProcessedUserString(const String& string) const;

		template <typename T>
		Vector<LocalisedString> getVector(const String& keyPrefix, const T& keys) const
		{
			Vector<LocalisedString> result;
			for (auto& k: keys) {
				result.push_back(get(keyPrefix + k));
			}
			return result;
		}

		const I18NLanguage& getCurrentLanguage() const;
		int getVersion() const;
		
		char getDecimalSeparator() const;

	private:
		I18NLanguage currentLanguage;
		std::optional<I18NLanguage> fallbackLanguage;
		std::map<I18NLanguage, std::map<String, String>> strings;
		std::map<String, ConfigObserver> observers;
		int version = 0;

		void loadLocalisation(const ConfigNode& node);
	};
}

