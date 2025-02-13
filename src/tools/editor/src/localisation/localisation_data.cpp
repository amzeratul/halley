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

void LocOriginalData::setLanguage(I18NLanguage language)
{
	this->language = language;
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

LocOriginalDataChunk& LocOriginalData::getChunk(const String& name)
{
	for (auto& chunk: chunks) {
		if (chunk.name == name) {
			return chunk;
		}
	}
	auto& chunk = chunks.emplace_back();
	chunk.name = name;
	return chunk;
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
	return tryGetVersion(key).value_or(0);
}

std::optional<int32_t> LocOriginalData::tryGetVersion(const String& key) const
{
	const auto iter = keyMap.find(key);
	if (iter != keyMap.end()) {
		return getEntry(iter->second).version;
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
			result.keyMap[entry.key] = static_cast<int32_t>(result.keyIndices.size());
			result.keyIndices.emplace_back(result.chunks.size() - 1, i++);
		}
	}

	return result;
}

void LocOriginalData::indexData()
{
	keyIndices.clear();
	keyMap.clear();

	const auto nChunks = chunks.size();
	for (size_t j = 0; j < nChunks; ++j) {
		const auto n = chunks[j].getNumEntries();
		for (size_t i = 0; i < n; ++i) {
			keyMap[chunks[j].getEntry(i).key] = static_cast<int32_t>(keyIndices.size());
			keyIndices.emplace_back(j, i);
		}
	}
}

LocTranslationEntry::LocTranslationEntry(const ConfigNode& node)
{
	value = node["value"].asString("");
	version = node["version"].asInt(0);
	origVersion = node["origVersion"].asInt(0);
}

LocTranslationEntry::LocTranslationEntry(String value, int32_t origVersion)
	: value(std::move(value))
	, origVersion(origVersion)
{
}

bool LocTranslationEntry::operator==(const LocTranslationEntry& other) const
{
	return value == other.value && version == other.version && origVersion == other.origVersion;
}

bool LocTranslationEntry::operator!=(const LocTranslationEntry& other) const
{
	return !(*this == other);
}

ConfigNode LocTranslationEntry::toConfigNode() const
{
	ConfigNode result = ConfigNode::MapType();
	result["value"] = value;
	result["version"] = version;
	result["origVersion"] = origVersion;
	return result;
}

LocTranslationData::LocTranslationData(const ConfigNode& node)
{
	load(node);
}

void LocTranslationData::load(const ConfigNode& node)
{
	language = I18NLanguage(node["language"].asString());
	entries = node["entries"].asHashMap<String, LocTranslationEntry>();
}

ConfigNode LocTranslationData::toConfigNode() const
{
	ConfigNode result = ConfigNode::MapType();
	result["language"] = language.getISOCode();
	result["entries"] = entries;
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
		if (auto version = original.tryGetVersion(entry.first)) {
			if (version == entry.second.origVersion) {
				result.translatedKeys++;
			} else {
				result.outdatedKeys++;
			}
		} else {
			result.obsoleteKeys++;
		}
	}

	return result;
}

LocTranslationData LocTranslationData::generateFromProject(const I18NLanguage& language, Project& project)
{
	LocTranslationData result;
	result.language = language;

	for (const auto& [name, data]: LocOriginalData::getProjectLocData(language, project)) {
		for (const auto& entry: data.asSequence()) {
			result.entries[entry["key"].asString()] = LocTranslationEntry { entry["value"].asString(""), 0 };
		}
	}

	return result;
}

bool LocTranslationData::updateFromRemote(const LocTranslationData& remote)
{
	if (remote.language != language) {
		throw Exception("Language mismatch: local is " + language.getISOCode() + ", remote is " + remote.language.getISOCode(), HalleyExceptions::Tools);
	}

	int nModified = 0;
	for (const auto& [key, remoteEntry]: remote.entries) {
		auto iter = entries.find(key);
		if (iter == entries.end()) {
			iter = entries.insert_or_assign(key, LocTranslationEntry()).first;
		}
		auto& myEntry = iter->second;

		if (remoteEntry.version > myEntry.version || myEntry.value.isEmpty()) {
			if (remoteEntry != myEntry) {
				myEntry = remoteEntry;
				++nModified;
			}
		}
	}

	Logger::logInfo("Updated " + toString(nModified) + " keys in " + language.getISOCode() + " from remote strings");

	return nModified > 0;
}

LocStringSet::LocStringSet(const ConfigNode& node)
{
	highestVersion = node["highestVersion"].asInt(0);
	if (node.hasKey("originalLanguage")) {
		//originalLanguage = LocOriginalData(node["originalLanguage"]); // Not implemented
	}
	localised = node["localised"].asHashMap<String, LocTranslationData>();
}

ConfigNode LocStringSet::toConfigNode() const
{
	ConfigNode result = ConfigNode::MapType();
	result["highestVersion"] = highestVersion;
	if (originalLanguage) {
		//result["originalLanguage"] = *originalLanguage; // Not implemented
	}
	result["localised"] = localised;
	return result;
}

LocTranslationData& LocStringSet::getLocalised(const I18NLanguage& language)
{
	const auto code = language.getISOCode();
	if (const auto iter = localised.find(code); iter != localised.end()) {
		return iter->second;
	}
	LocTranslationData data;
	data.language = language;
	localised[code] = std::move(data);
	return localised.at(code);
}
