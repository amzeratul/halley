#pragma once

#include "halleystring.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/utils/hash.h"

namespace Halley {
   	class ConfigNode;

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
		I18NLanguage(const ConfigNode& node);

		ConfigNode toConfigNode() const;

		void set(String languageCode, std::optional<String> countryCode);

		const String& getLanguageCode() const;
		const std::optional<String>& getCountryCode() const;
		String getISOCode() const;
		char getDecimalSeparator() const;

		I18NLanguageMatch getMatch(const I18NLanguage& other) const;

		static std::optional<I18NLanguage> getBestMatch(const Vector<I18NLanguage>& languages, const I18NLanguage& target, std::optional<I18NLanguage> fallback = {});

		bool operator==(const I18NLanguage& other) const;
		bool operator!=(const I18NLanguage& other) const;
		bool operator<(const I18NLanguage& other) const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

	private:
		String languageCode;
		std::optional<String> countryCode;
	};

}

namespace std {
	template <>
	struct hash<Halley::I18NLanguage> {
	public:
		size_t operator()(const Halley::I18NLanguage& lang) const
		{
			Halley::Hash::Hasher hasher;
			hasher.feed(lang.getLanguageCode());
			if (lang.getCountryCode()) {
				hasher.feed(*lang.getCountryCode());
			}
			return hasher.digest();
		}
	};
}