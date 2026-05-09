#pragma once

#include <map>
#include "halleystring.h"
#include "i18n_language.h"
#include "localised_string.h"

namespace Halley {
	class StringOutputServer;
	class ConfigNode;
	class ConfigFile;
	class ConfigObserver;
	class I18N;
	class I18NLanguage;
	class Font;

	struct StringOutputMetrics;
	enum class StringOutputType;
	class II18N {
	public:
		virtual ~II18N() = default;

		virtual LocalisedString get(const String& key) const = 0;
		virtual std::optional<LocalisedString> tryGet(const String& key) const = 0;
	};

	class I18N : public II18N {
	public:
		I18N();
		I18N(Resources& resources, I18NLanguage currentLanguage = I18NLanguage("en-GB"), std::optional<I18NLanguage> fallbackLanguage = {});
		~I18N();

		void update();
		void loadStrings(Resources& resources);
		void loadLocalisationFile(const ConfigFile& config);

		void updateStrings(const I18NLanguage& language, HashMap<String, String> strings);

		void setCurrentLanguage(I18NLanguage language);
		const I18NLanguage& getCurrentLanguage() const;

		void setFallbackLanguage(std::optional<I18NLanguage> language);
		const std::optional<I18NLanguage>& getFallbackLanguage() const;
		
		void setSecondaryLanguage(std::optional<I18NLanguage> language);
		const std::optional<I18NLanguage>& getSecondaryLanguage() const;

		const I18NLanguage& getLanguageFromIndex(int languageIdx) const;
		Vector<I18NLanguage> getLanguagesAvailable() const;

		LocalisedString get(const String& key) const override;
		std::optional<LocalisedString> tryGet(const String& key) const override;
		LocalisedString get(const String& key, const I18NLanguage& language) const;
		std::optional<LocalisedString> tryGet(const String& key, const I18NLanguage& language) const;
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

		int getVersion() const;
		
		char getDecimalSeparator() const;

		void checkForDuplicatedStrings(const Vector<String>& ignoredPrefixes = {}) const;
		Vector<uint32_t> getCodepointsUsedBy(const I18NLanguage& language) const;
		void checkForCodepointsInFonts(gsl::span<const std::shared_ptr<const Font>> fonts) const;

		bool createStringOutputServer(const HalleyAPI& api, const String& host, int port);
		StringOutputServer* tryGetStringOutputServer() const;

	private:
		struct LangData {
			HashMap<String, String> strings;
			int index = 0;
		};

		I18NLanguage currentLanguage;
		std::optional<I18NLanguage> fallbackLanguage;
		std::optional<I18NLanguage> secondaryLanguage;

		HashMap<I18NLanguage, LangData> strings;
		Vector<I18NLanguage> languageIndices;

		HashMap<String, ConfigObserver> observers;
		int version = 0;

		std::unique_ptr<StringOutputServer> stringOutputServer;

		LangData& getLanguageData(const I18NLanguage& language);
		void loadLocalisation(const ConfigNode& node, const String& assetId, bool allowUpdating);
		int getLanguageIndex(const I18NLanguage& language) const;
	};

	class I18NVersionChecker {
	public:
		I18NVersionChecker() = default;
		I18NVersionChecker(const I18N& i18n);

		void setI18N(const I18N& i18n);

		bool checkChanged();

	private:
		const I18N* i18n = nullptr;
		int version = -1;
	};
}

