#include "localisation_data.h"

using namespace Halley;

LocalisationStats& LocalisationStats::operator+=(const LocalisationStats& other)
{
	totalWords += other.totalWords;
	totalKeys += other.totalKeys;
	for (auto& [k, v]: other.keysPerCategory) {
		keysPerCategory[k] += v;
	}
	for (auto& [k, v]: other.wordsPerCategory) {
		wordsPerCategory[k] += v;
	}

	return *this;
}

LocalisationDataValue::LocalisationDataValue(String value)
	: value(std::move(value))
{
	computeHash();
}

void LocalisationDataValue::computeHash()
{
	// TODO
	hash = 0;
}

LocalisationDataEntry::LocalisationDataEntry(String key, String value, String context, String comment)
	: key(std::move(key))
	, context(std::move(context))
	, comment(std::move(comment))
{
	values.push_back(std::move(value));
}

namespace {
	int getWordCount(const String& line)
	{
		const char* delims = " ,;.?![]{}()";
		auto start = delims;
		auto end = delims + strlen(delims);

		bool isInWord = false;
		int count = 0;

		for (auto c: line.cppStr()) {
			const bool isWordCharacter = std::find(start, end, c) == end;
			if (isWordCharacter && !isInWord) {
				++count;
			}
			isInWord = isWordCharacter;
		}

		return count;
	}
}

LocalisationStats LocalisationDataChunk::getStats() const
{
	LocalisationStats result;
	for (const auto& entry: entries) {
		const auto wordCount = getWordCount(entry.values.back().value);
		result.totalKeys++;
		result.keysPerCategory[category]++;
		result.totalWords += wordCount;
		result.wordsPerCategory[category] += wordCount;
	}
	return result;
}

bool LocalisationDataChunk::operator<(const LocalisationDataChunk& other) const
{
	return name < other.name;
}

void LocalisationDataChunk::computeHash()
{
	// TODO
	hash = 0;
}

LocalisationStats LocalisationData::getStats() const
{
	LocalisationStats result;
	for (auto& chunk: chunks) {
		result += chunk.getStats();
	}
	return result;
}

TranslationStats LocalisationData::getTranslationStats(const LocalisationData& original) const
{
	TranslationStats result;
	
	HashMap<String, LocalisationHashType> origKeys;
	for (const auto& chunk: original.chunks) {
		for (const auto& entry: chunk.entries) {
			const auto& value = entry.values.back();
			origKeys[value.value] = value.hash;
		}
	}

	for (const auto& chunk: chunks) {
		for (const auto& entry: chunk.entries) {
			const auto& value = entry.values.back();
			const auto iter = origKeys.find(value.value);

			if (iter != origKeys.end()) {
				if (iter->second == value.hash) {
					result.translatedKeys++;
				} else {
					result.outdatedKeys++;
				}
			}
		}
	}

	return result;
}

LocalisationData LocalisationData::generateFromResources(const I18NLanguage& language, Resources& resources, const ILocalisationInfoRetriever& infoRetriever)
{
	LocalisationData result;
	result.language = language;

	// Scan for language
	for (auto& assetName: resources.enumerate<ConfigFile>()) {
		if (assetName.startsWith("strings/")) {
			auto& config = *resources.get<ConfigFile>(assetName);

			for (auto& languageNode: config.getRoot().asMap()) {
				const auto curLang = I18NLanguage(languageNode.first);
				if (curLang == language) {
					result.chunks.push_back(generateChunk(assetName.mid(8), languageNode.second, infoRetriever));
				}
			}
		}
	}

	return result;
}

LocalisationDataChunk LocalisationData::generateChunk(String name, const ConfigNode& data, const ILocalisationInfoRetriever& infoRetriever)
{
	LocalisationDataChunk result;
	result.category = infoRetriever.getCategory(name);
	result.name = std::move(name);

	// TODO: ConfigNode map is not ordered, should probably parse raw YAML here
	for (const auto& [key, value]: data.asMap()) {
		String context; // TODO
		String comment; // TODO
		result.entries.emplace_back(key, value.asString(""), std::move(context), std::move(comment));
	}

	result.computeHash();

	return result;
}
