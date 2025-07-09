#pragma once

#include "halley/text/halleystring.h"
#include "halley/data_structures/vector.h"
#include "halley/data_structures/hash_map.h"
#include "halley/tools/project/project_loc_options.h"

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
		int readyWords = 0;
		int readyKeys = 0;
		HashMap<String, int> wordsPerCategory;
		HashMap<String, int> keysPerCategory;
		HashMap<String, int> wordsPerKey;

		LocalisationStats& operator+=(const LocalisationStats& other);

		static int getWordCount(const String& line);
	};

	class TranslationStats {
	public:
		int translatedKeys = 0;
		int outdatedKeys = 0;
		int obsoleteKeys = 0;
		int translatedWords = 0;
		int outdatedWords = 0;
	};

	enum class LocReadyStatus {
		Ready,
		NotReady
	};

	template <>
	struct EnumNames<LocReadyStatus> {
		constexpr std::array<const char*, 2> operator()() const {
			return{{
				"ready",
				"notReady"
			}};
		}
	};

	class LocalisationDataEntry {
	public:
		String key;
		String value;
		String context;
		String comment;
		int version = 0;
		LocPriority priority = LocPriority::Normal;

		LocalisationDataEntry() = default;
		LocalisationDataEntry(String key, String value, String context = "", String comment = "", LocPriority priority = LocPriority::Normal);

		LocReadyStatus getReadyState(const LocalisationFilterRules& rules) const;
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

		LocalisationStats getStats(const LocalisationFilterRules& filterRules) const;
		LocalisationStats getStats(const LocTranslationData& translated, const LocalisationFilterRules& filterRules) const;

		size_t getNumEntries() const override;
		const LocalisationDataEntry& getEntry(size_t idx) const override;
		LocalisationDataEntry& getEntry(size_t idx);

		bool operator<(const LocOriginalDataChunk& other) const;
	};

	class LocStringProperties {
	public:
		String key;
		std::optional<String> comment;
		std::optional<String> context;
		std::optional<LocPriority> priority;

		LocStringProperties() = default;
		LocStringProperties(const LocalisationDataEntry& from, const LocalisationDataEntry& to);

		void apply(LocalisationDataEntry& entry) const;
		bool hasChange() const;

		ConfigNode toConfigNode() const;
	};

	class LocOriginalData : public ILocOriginalData {
	public:
		void setLanguage(I18NLanguage language);
		const I18NLanguage& getLanguage() const;
		LocalisationStats getStats(const LocalisationFilterRules& filterRules) const;

		LocOriginalDataChunk& getChunk(const String& name);
		const LocOriginalDataChunk* tryGetChunk(const String& name) const;
		const Vector<LocOriginalDataChunk>& getChunks() const;

		int32_t getVersion(const String& key) const;
		std::optional<int32_t> tryGetVersion(const String& key) const;
		bool hasKey(const String& key) const;

		size_t getNumEntries() const override;
		const LocalisationDataEntry& getEntry(size_t idx) const override;
		LocalisationDataEntry& getEntry(size_t idx);
		LocalisationDataEntry* tryGetEntry(const String& key);
		const LocalisationDataEntry* tryGetEntry(const String& key) const;

		bool setValue(const String& key, const String& value);

		Vector<LocStringProperties> makeStringPropertiesDelta(const LocOriginalData& remote, const Vector<String>& keysModified) const;
		void applyStringProperties(const Vector<LocStringProperties>& entries);

		static Vector<std::pair<String, ConfigNode>> getProjectLocData(const I18NLanguage& language, Project& project);
		static LocOriginalData generateFromProject(const I18NLanguage& language, Project& project, const ILocalisationInfoRetriever& infoRetriever);

		bool update(const LocOriginalData& other);
		bool updateLocalFromRemote(const LocOriginalData& remote);

		void indexData();

	private:
		I18NLanguage language;
		Vector<LocOriginalDataChunk> chunks;
		HashMap<String, int32_t> keyMap;
		Vector<std::pair<size_t, size_t>> keyIndices;
	};

	class LocTranslationEntry {
	public:
		String value;
		int32_t version = -1;
		int32_t origVersion = -1;

		LocTranslationEntry() = default;
		LocTranslationEntry(const ConfigNode& node);
		LocTranslationEntry(String value, int32_t origVersion);

		bool operator==(const LocTranslationEntry& other) const;
		bool operator!=(const LocTranslationEntry& other) const;

		ConfigNode toConfigNode() const;
	};

	class LocTranslationData {
	public:
		I18NLanguage language;
		HashMap<String, LocTranslationEntry> entries;

		LocTranslationData() = default;
		LocTranslationData(const ConfigNode& node);

		void load(const ConfigNode& node);
		ConfigNode toConfigNode() const;

		bool setValue(const String& key, int32_t curVersion, String value);
		bool setVersion(const String& key, int32_t curVersion);
		const LocTranslationEntry* tryGetEntry(const String& key) const;

		TranslationStats getTranslationStats(const LocOriginalData& original, const LocalisationStats& origStats) const;
		static LocTranslationData generateFromProject(const I18NLanguage& language, Project& project);

		bool update(const LocTranslationData& remote);
		bool updateLocalFromRemote(const LocTranslationData& remote);
		void updateOriginalVersions(const LocOriginalData& originalLanguage);
		bool pruneKeys(const LocOriginalData& originalLanguage);

		LocTranslationData makeDeltaFrom(const LocTranslationData& other) const;
		LocTranslationData makeDeltaFrom(const LocTranslationData& other, gsl::span<const String> keys) const;

		bool operator==(const LocTranslationData& other) const;
		bool operator!=(const LocTranslationData& other) const;
	};

	class LocStringSet {
	public:
		std::optional<LocOriginalData> originalLanguage;
		HashMap<String, LocTranslationData> localised;
		int highestVersion = 0;

		LocStringSet() = default;
		LocStringSet(const ConfigNode& node);

		ConfigNode toConfigNode() const;

		LocTranslationData& getLocalised(const I18NLanguage& language);
		bool updateWith(const LocStringSet& other);
	};
}
