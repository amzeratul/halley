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
		String value;
		String context;
		String comment;
		int version = 0;

		LocalisationDataEntry() = default;
		LocalisationDataEntry(String key, String value, String context = "", String comment = "");
	};

	class ILocOriginalData {
	public:
		virtual ~ILocOriginalData() = default;

		virtual size_t getNumEntries() const = 0;
		virtual const LocalisationDataEntry& getEntry(size_t idx) const = 0;
	};

	class LocOriginalDataChunk : public ILocOriginalData {
	public:
		String name;
		String category;
		Vector<LocalisationDataEntry> entries;
		LocalisationHashType hash = 0;

		LocalisationStats getStats() const;
		LocalisationStats getStats(const LocTranslationData& translated) const;

		size_t getNumEntries() const override;
		const LocalisationDataEntry& getEntry(size_t idx) const override;

		void computeHash();

		bool operator<(const LocOriginalDataChunk& other) const;
	};

	class LocOriginalData : public ILocOriginalData {
	public:
		void setLanguage(I18NLanguage language);
		const I18NLanguage& getLanguage() const;
		LocalisationStats getStats() const;

		LocOriginalDataChunk& getChunk(const String& name);
		const LocOriginalDataChunk* tryGetChunk(const String& name) const;
		const Vector<LocOriginalDataChunk>& getChunks() const;

		int32_t getVersion(const String& key) const;
		std::optional<int32_t> tryGetVersion(const String& key) const;

		size_t getNumEntries() const override;
		const LocalisationDataEntry& getEntry(size_t idx) const override;

		static Vector<std::pair<String, ConfigNode>> getProjectLocData(const I18NLanguage& language, Project& project);
		static LocOriginalData generateFromProject(const I18NLanguage& language, Project& project, const ILocalisationInfoRetriever& infoRetriever);

		void indexData();

	private:
		I18NLanguage language;
		Vector<LocOriginalDataChunk> chunks;
		HashMap<String, int32_t> keyVersions;
		Vector<std::pair<size_t, size_t>> keyIndices;
	};

	class LocTranslationEntry {
	public:
		String value;
		int32_t origVersion;
	};

	class LocTranslationData {
	public:
		I18NLanguage language;
		HashMap<String, LocTranslationEntry> entries;

		void setValue(const String& key, int32_t curVersion, String value);
		const LocTranslationEntry* tryGetEntry(const String& key) const;

		TranslationStats getTranslationStats(const LocOriginalData& original) const;
		static LocTranslationData generateFromProject(const I18NLanguage& language, Project& project);
	};
}
