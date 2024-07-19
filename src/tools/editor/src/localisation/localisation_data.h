#pragma once

#include "halley/text/halleystring.h"
#include "halley/data_structures/vector.h"
#include "halley/data_structures/hash_map.h"

namespace Halley {
	using LocalisationHashType = uint64_t;

	class ILocalisationInfoRetriever {
	public:
		virtual ~ILocalisationInfoRetriever() = default;

		virtual String getCategory(const String& assetId) const = 0;
	};

	class LocalisationStats {
	public:
		int totalWords = 0;
		int totalKeys = 0;
		HashMap<String, int> wordsPerCategory;
		HashMap<String, int> keysPerCategory;

		LocalisationStats& operator+=(const LocalisationStats& other);
	};

	class TranslationStats {
	public:
		int translatedKeys = 0;
		int outdatedKeys = 0;
	};

	class LocalisationDataValue {
	public:
		String value;
		LocalisationHashType hash = 0;

		LocalisationDataValue() = default;
		LocalisationDataValue(String value);

		void computeHash();
	};

	class LocalisationDataEntry {
	public:
		String key;
		Vector<LocalisationDataValue> values;
		String context;
		String comment;

		LocalisationDataEntry() = default;
		LocalisationDataEntry(String key, String value, String context = "", String comment = "");
	};

	class LocalisationDataChunk {
	public:
		String name;
		String category;
		Vector<LocalisationDataEntry> entries;
		LocalisationHashType hash = 0;

		LocalisationStats getStats() const;

		bool operator<(const LocalisationDataChunk& other) const;

		void computeHash();
	};

	class LocalisationData {
	public:
		I18NLanguage language;
		Vector<LocalisationDataChunk> chunks;

		LocalisationStats getStats() const;
		TranslationStats getTranslationStats(const LocalisationData& original) const;

		static LocalisationData generateFromResources(const I18NLanguage& language, Resources& resource, const ILocalisationInfoRetriever& infoRetriever);

	private:
		static LocalisationDataChunk generateChunk(String name, const ConfigNode& data, const ILocalisationInfoRetriever& infoRetriever);
	};
}
