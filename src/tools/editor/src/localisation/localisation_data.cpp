#include "localisation_data.h"
#include "halley/tools/file/filesystem_cache.h"
#include "halley/tools/project/project.h"

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

LocalisationDataEntry::LocalisationDataEntry(String key, String value, String context, String comment)
	: key(std::move(key))
	, value(std::move(value))
	, context(std::move(context))
	, comment(std::move(comment))
{
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

LocalisationStats LocOriginalDataChunk::getStats() const
{
	LocalisationStats result;
	for (const auto& entry: entries) {
		const auto wordCount = getWordCount(entry.value);
		result.totalKeys++;
		result.keysPerCategory[category]++;
		result.totalWords += wordCount;
		result.wordsPerCategory[category] += wordCount;
	}
	return result;
}

LocalisationStats LocOriginalDataChunk::getStats(const LocTranslationData& translated) const
{
	LocalisationStats result;
	for (const auto& entry: entries) {
		if (const auto* translatedEntry = translated.tryGetEntry(entry.key)) {
			const auto wordCount = getWordCount(translatedEntry->value);
			result.totalKeys++;
			result.keysPerCategory[category]++;
			result.totalWords += wordCount;
			result.wordsPerCategory[category] += wordCount;
		}
	}
	return result;
}

size_t LocOriginalDataChunk::getNumEntries() const
{
	return entries.size();
}

const LocalisationDataEntry& LocOriginalDataChunk::getEntry(size_t idx) const
{
	return entries[idx];
}

bool LocOriginalDataChunk::operator<(const LocOriginalDataChunk& other) const
{
	return name < other.name;
}

void LocOriginalDataChunk::computeHash()
{
	// TODO
	hash = 0;
}

const I18NLanguage& LocOriginalData::getLanguage() const
{
	return language;
}

LocalisationStats LocOriginalData::getStats() const
{
	LocalisationStats result;
	for (auto& chunk: chunks) {
		result += chunk.getStats();
	}
	return result;
}

const LocOriginalDataChunk* LocOriginalData::tryGetChunk(const String& name) const
{
	for (auto& chunk: chunks) {
		if (chunk.name == name) {
			return &chunk;
		}
	}
	return nullptr;
}

const Vector<LocOriginalDataChunk>& LocOriginalData::getChunks() const
{
	return chunks;
}

int32_t LocOriginalData::getVersion(const String& key) const
{
	const auto iter = keyVersions.find(key);
	if (iter != keyVersions.end()) {
		return iter->second;
	}
	return 0;
}

std::optional<int32_t> LocOriginalData::tryGetVersion(const String& key) const
{
	const auto iter = keyVersions.find(key);
	if (iter != keyVersions.end()) {
		return iter->second;
	}
	return std::nullopt;
}

size_t LocOriginalData::getNumEntries() const
{
	return keyIndices.size();
}

const LocalisationDataEntry& LocOriginalData::getEntry(size_t idx) const
{
	const auto index = keyIndices[idx];
	return chunks[index.first].getEntry(index.second);
}

namespace {
	LocOriginalDataChunk generateChunk(String name, const ConfigNode& data, const ILocalisationInfoRetriever& infoRetriever)
	{
		LocOriginalDataChunk result;
		result.category = infoRetriever.getCategory(name);
		result.name = std::move(name);

		for (const auto& entry: data.asSequence()) {
			String context; // TODO
			String comment; // TODO
			result.entries.emplace_back(entry["key"].asString(), entry["value"].asString(""), std::move(context), std::move(comment));
		}

		result.computeHash();

		return result;
	}
}

Vector<std::pair<String, ConfigNode>> LocOriginalData::getProjectLocData(const I18NLanguage& language, Project& project)
{
	Vector<std::pair<String, ConfigNode>> result;

	auto suffix = language.getISOCode();

	const auto& rootPath = project.getAssetsSrcPath() / "config" / "strings";
	for (const auto& assetName: project.getFileSystemCache().enumerateDirectory(rootPath)) {
		if (!assetName.getString().contains(suffix)) {
			continue;
		}

		const auto data = Path::readFile(rootPath / assetName);
		if (!data.empty()) {
			const auto config = YAMLConvert::parseConfig(data, YAMLConvert::ParseOptions{ true });

			for (auto& languageNode: config.getRoot().asSequence()) {
				const auto curLang = I18NLanguage(languageNode["key"].asString());
				if (curLang == language) {
					auto chunkName = assetName.replaceExtension("").getString(false).replaceAll("-" + suffix, "").replaceAll("_" + suffix, "");
					result.emplace_back(chunkName, languageNode["value"]);
				}
			}
		}
	}

	return result;
}

LocOriginalData LocOriginalData::generateFromProject(const I18NLanguage& language, Project& project, const ILocalisationInfoRetriever& infoRetriever)
{
	LocOriginalData result;
	result.language = language;

	for (const auto& [name, data]: getProjectLocData(language, project)) {
		result.chunks.push_back(generateChunk(name, data, infoRetriever));

		size_t i = 0;
		for (const auto& entry: result.chunks.back().entries) {
			result.keyVersions[entry.key] = entry.version;
			result.keyIndices.emplace_back(result.chunks.size() - 1, i++);
		}
	}

	return result;
}

void LocTranslationData::setValue(const String& key, int32_t curVersion, String value)
{
	entries[key] = LocTranslationEntry{ std::move(value), curVersion };
}

const LocTranslationEntry* LocTranslationData::tryGetEntry(const String& key) const
{
	const auto iter = entries.find(key);
	if (iter != entries.end()) {
		return &iter->second;
	}
	return nullptr;
}

TranslationStats LocTranslationData::getTranslationStats(const LocOriginalData& original) const
{
	TranslationStats result;

	for (const auto& entry: entries) {
		auto version = original.tryGetVersion(entry.first);
		if (version) {
			if (version == entry.second.origVersion) {
				result.translatedKeys++;
			} else {
				result.outdatedKeys++;
			}
		}
	}

	return result;
}

LocTranslationData LocTranslationData::generateFromProject(const I18NLanguage& language, Project& project)
{
	LocTranslationData result;

	for (const auto& [name, data]: LocOriginalData::getProjectLocData(language, project)) {
		for (const auto& entry: data.asSequence()) {
			result.entries[entry["key"].asString()] = LocTranslationEntry { entry["value"].asString(""), 0 };
		}
	}

	return result;
}
