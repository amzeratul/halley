#pragma once

#include "halley/text/halleystring.h"
#include "halley/data_structures/vector.h"
#include "halley/data_structures/hash_map.h"

namespace Halley {
	using LocalisationHashType = uint64_t;
	class LocTranslationData;

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
	};;

	class LocalisationDataEntry {
	public:
		String key;
		LocalisationHashType hash = 0;
		String value;
		String context;
		String comment;

		LocalisationDataEntry() = default;
		LocalisationDataEntry(String key, String value, String context = "", String comment = "");

		void computeHash();
	};

	class LocOriginalDataChunk {
	public:
		String name;
		String category;
		Vector<LocalisationDataEntry> entries;
		LocalisationHashType hash = 0;

		LocalisationStats getStats() const;
		LocalisationStats getStats(const LocTranslationData& translated) const;

		bool operator<(const LocOriginalDataChunk& other) const;

		void computeHash();
	};

	class LocOriginalData {
	public:
		I18NLanguage language;
		Vector<LocOriginalDataChunk> chunks;
		HashMap<String, LocalisationHashType> keyVersions;

		LocalisationStats getStats() const;

		LocOriginalDataChunk& getChunk(const String& name);
		LocOriginalDataChunk* tryGetChunk(const String& name);
		const LocOriginalDataChunk* tryGetChunk(const String& name) const;

		LocalisationHashType getVersion(const String& key) const;

		static Vector<std::pair<String, ConfigNode>> getProjectLocData(const I18NLanguage& language, Project& project);
		static LocOriginalData generateFromProject(const I18NLanguage& language, Project& project, const ILocalisationInfoRetriever& infoRetriever);
	};

	class LocTranslationEntry {
	public:
		String value;
		LocalisationHashType origVersion;
	};

	class LocTranslationData {
	public:
		I18NLanguage language;
		HashMap<String, LocTranslationEntry> entries;

		void setValue(const String& key, LocalisationHashType curVersion, String value);
		const LocTranslationEntry* tryGetEntry(const String& key) const;

		TranslationStats getTranslationStats(const LocOriginalData& original) const;
		static LocTranslationData generateFromProject(const I18NLanguage& language, Project& project);
	};
}
