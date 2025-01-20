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

void LocalisationDataEntry::computeHash()
{
	// TODO
	hash = 0;
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
		const auto wordCount = getWordCount(entry.value);
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

void LocalisationDataChunk::alignWith(const LocalisationDataChunk& origData)
{
	name = origData.name;
	category = origData.category;

	HashMap<String, size_t> prevEntries;
	for (size_t i = 0; i < entries.size(); ++i) {
		prevEntries[entries[i].key] = i;
	}

	Vector<LocalisationDataEntry> newEntries;
	newEntries.resize(origData.entries.size());
	for (size_t i = 0; i < origData.entries.size(); ++i) {
		if (const auto iter = prevEntries.find(origData.entries[i].key); iter != prevEntries.end()) {
			newEntries[i] = entries[iter->second];
		} else {
			newEntries[i].key = origData.entries[i].key;
		}
	}

	entries = std::move(newEntries);
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
	const auto& origKeys = original.keyHashes;

	for (const auto& keyHash: keyHashes) {
		const auto iter = origKeys.find(keyHash.first);

		if (iter != origKeys.end()) {
			if (iter->second == keyHash.second) {
				result.translatedKeys++;
			} else {
				result.outdatedKeys++;
			}
		}
	}

	return result;
}

LocalisationDataChunk& LocalisationData::getChunk(const String& name)
{
	for (auto& chunk: chunks) {
		if (chunk.name == name) {
			return chunk;
		}
	}
	throw Exception("Chunk not found: " + name, HalleyExceptions::Tools);
}

LocalisationDataChunk* LocalisationData::tryGetChunk(const String& name)
{
	for (auto& chunk: chunks) {
		if (chunk.name == name) {
			return &chunk;
		}
	}
	return nullptr;
}

const LocalisationDataChunk* LocalisationData::tryGetChunk(const String& name) const
{
	for (auto& chunk: chunks) {
		if (chunk.name == name) {
			return &chunk;
		}
	}
	return nullptr;
}

void LocalisationData::alignWith(const LocalisationData& original)
{
	// Store original entries
	HashMap<String, LocalisationDataEntry> entries;
	for (auto& chunk: chunks) {
		for (auto& entry: chunk.entries) {
			const auto key = entry.key;
			entries[key] = std::move(entry);
		}
	}

	// Copy chunks from original
	chunks = original.chunks;

	// Replace entry data
	for (auto& chunk: chunks) {
		auto origEntries = std::move(chunk.entries);
		chunk.entries.clear();

		for (auto& entry: origEntries) {
			const auto iter = entries.find(entry.key);
			if (iter != entries.end()) {
				chunk.entries.push_back(std::move(iter->second));
			}
		}
		chunk.computeHash();
	}
}

void LocalisationData::setValue(const String& key, String value)
{
	// TODO: this code is awful
	for (auto& chunk: chunks) {
		for (auto& entry: chunk.entries) {
			if (entry.key == key) {
				entry.value = std::move(value);
				return;
			}
		}
	}
}

namespace {
	LocalisationDataChunk generateChunk(String name, const ConfigNode& data, const ILocalisationInfoRetriever& infoRetriever)
	{
		LocalisationDataChunk result;
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

LocalisationData LocalisationData::generateFromProject(const I18NLanguage& language, Project& project, const ILocalisationInfoRetriever& infoRetriever)
{
	LocalisationData result;
	result.language = language;

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
					result.chunks.push_back(generateChunk(std::move(chunkName), languageNode["value"], infoRetriever));
				}
			}
		}
	}

	for (const auto& chunk: result.chunks) {
		for (const auto& entry: chunk.entries) {
			result.keyHashes[entry.value] = entry.hash;
		}
	}

	return result;
}
