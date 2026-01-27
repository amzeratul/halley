#include "localisation_data.h"
#include "halley/tools/file/filesystem_cache.h"
#include "halley/tools/project/project.h"

using namespace Halley;

LocalisationStats& LocalisationStats::operator+=(const LocalisationStats& other)
{
	totalWords += other.totalWords;
	totalKeys += other.totalKeys;
	readyWords += other.readyWords;
	readyKeys += other.readyKeys;
	for (auto& [k, v]: other.keysPerCategory) {
		keysPerCategory[k] += v;
	}
	for (auto& [k, v]: other.wordsPerCategory) {
		wordsPerCategory[k] += v;
	}
	for (auto& [k, v]: other.readyPerCategory) {
		readyPerCategory[k] += v;
	}
	for (auto& [k, v]: other.wordsPerKey) {
		wordsPerKey[k] = v;
	}

	return *this;
}

int LocalisationStats::getWordCount(const String& line)
{
	constexpr char32_t delims[] = U" ,;.?!:\"¿¡[]{}()\n\t";
	const auto* start = std::begin(delims);
	const auto* end = std::end(delims);

	bool isInWord = false;
	int count = 0;

	for (auto c: line.getUTF32()) {
		const bool isWordCharacter = std::find(start, end, c) == end;
		if (isWordCharacter && !isInWord) {
			++count;
		}
		isInWord = isWordCharacter;
	}

	return count;
}

const String& LocalisationDataEntry::getKey() const
{
	return key;
}

const String& LocalisationDataEntry::getKeyLowercase() const
{
	if (!keyLowercase) {
		keyLowercase = key.asciiLower();
	}
	return *keyLowercase;
}

void LocalisationDataEntry::setKey(String key)
{
	this->key = std::move(key);
	keyLowercase = {};
}

const String& LocalisationDataEntry::getValue() const
{
	return value;
}

const String& LocalisationDataEntry::getValueLowercase() const
{
	if (!valueLowercase) {
		valueLowercase = value.asciiLower();
	}
	return *valueLowercase;
}

void LocalisationDataEntry::setValue(String value)
{
	this->value = std::move(value);
	valueLowercase = {};
}

const String& LocalisationDataEntry::getContext() const
{
	return context;
}

void LocalisationDataEntry::setContext(String context)
{
	this->context = std::move(context);
}

const String& LocalisationDataEntry::getComment() const
{
	return comment;
}

void LocalisationDataEntry::setComment(String comment)
{
	this->comment = std::move(comment);
}

int LocalisationDataEntry::getVersion() const
{
	return version;
}

void LocalisationDataEntry::setVersion(int version)
{
	this->version = version;
}

LocPriority LocalisationDataEntry::getPriority() const
{
	return priority;
}

void LocalisationDataEntry::setPriority(LocPriority priority)
{
	this->priority = priority;
}

LocalisationDataEntry::LocalisationDataEntry(String key, String value, String context, String comment, LocPriority priority)
	: key(std::move(key))
	, value(std::move(value))
	, context(std::move(context))
	, comment(std::move(comment))
	, priority(priority)
{
}

LocReadyStatus LocalisationDataEntry::getReadyState(const LocalisationFilterRules& rules) const
{
	return priority >= rules.minPriorityForReady ? LocReadyStatus::Ready : LocReadyStatus::NotReady;
}

bool LocalisationDataEntry::matchesSearchString(const String& searchString, const LocTranslationEntry* translation, bool searchStringIsLowerCase) const
{
	return getKeyLowercase().contains(searchString, true)
		|| getValueLowercase().contains(searchString, true)
		|| (translation && translation->getValueLowercase().contains(searchString, true));
}

LocOriginalDataChunk::LocOriginalDataChunk(String name, String category, Vector<LocalisationDataEntry> entries)
	: name(std::move(name))
	, category(std::move(category))
	, entries(std::move(entries))
{
}

LocalisationStats LocOriginalDataChunk::getStats(const LocalisationFilterRules& filterRules) const
{
	LocalisationStats result;
	for (const auto& entry: entries) {
		const auto wordCount = LocalisationStats::getWordCount(entry.getValue());
		const bool ready = entry.getReadyState(filterRules) == LocReadyStatus::Ready;

		result.wordsPerKey[entry.getKey()] = wordCount;
		result.totalKeys++;
		result.keysPerCategory[category]++;
		result.totalWords += wordCount;
		result.wordsPerCategory[category] += wordCount;
		result.readyPerCategory[category] += ready ? wordCount : 0;
		result.readyWords += ready ? wordCount : 0;
	}
	return result;
}

LocalisationStats LocOriginalDataChunk::getStats(const LocTranslationData& translated, const LocalisationFilterRules& filterRules) const
{
	LocalisationStats result;
	for (const auto& entry: entries) {
		if (const auto* translatedEntry = translated.tryGetEntry(entry.getKey())) {
			const auto wordCount = LocalisationStats::getWordCount(translatedEntry->getValue());
			const bool ready = entry.getReadyState(filterRules) == LocReadyStatus::Ready;

			result.wordsPerKey[entry.getKey()] = wordCount;
			result.totalKeys++;
			result.keysPerCategory[category]++;
			result.totalWords += wordCount;
			result.wordsPerCategory[category] += wordCount;
			result.readyPerCategory[category] += ready ? wordCount : 0;
			result.readyWords += ready ? wordCount : 0;
		}
	}
	return result;
}

size_t LocOriginalDataChunk::getNumEntries() const
{
	return entries.size();
}

const String& LocOriginalDataChunk::getGroupNameEntry(size_t idx) const
{
	return name;
}

const LocalisationDataEntry& LocOriginalDataChunk::getEntry(size_t idx) const
{
	return entries.at(idx);
}

LocalisationDataEntry& LocOriginalDataChunk::getEntry(size_t idx)
{
	return entries.at(idx);
}

bool LocOriginalDataChunk::operator<(const LocOriginalDataChunk& other) const
{
	return name < other.name;
}

bool LocOriginalDataChunk::hasKeyValueChanges(const LocOriginalDataChunk& other) const
{
	if (entries.size() != other.entries.size()) {
		// Different number of keys
		return true;
	}

	HashMap<String, String> myValues;
	for (const auto& entry: entries) {
		myValues[entry.getKey()] = entry.getValue();
	}
	for (const auto& entry: other.entries) {
		auto iter = myValues.find(entry.getKey());
		if (iter != myValues.end()) {
			if (iter->second != entry.getValue()) {
				// Our key values don't match
				return true;
			}
		} else {
			// Other has a key I don't have
			return true;
		}
	}

	// No need to check if I have a key that other doesn't - if I did, our key numbers wouldn't match, since we checked for other having keys I don't
	return false;
}

void LocOriginalData::setLanguage(I18NLanguage language)
{
	this->language = language;
}

const I18NLanguage& LocOriginalData::getLanguage() const
{
	return language;
}

LocalisationStats LocOriginalData::getStats(const LocalisationFilterRules& filterRules) const
{
	LocalisationStats result;
	for (auto& chunk: chunks) {
		result += chunk.getStats(filterRules);
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
		return getEntry(iter->second).getVersion();
	}
	return std::nullopt;
}

bool LocOriginalData::hasKey(const String& key) const
{
	return keyMap.contains(key);
}

size_t LocOriginalData::getNumEntries() const
{
	return keyIndices.size();
}

const String& LocOriginalData::getGroupNameEntry(size_t idx) const
{
	const auto index = keyIndices[idx];
	return chunks[index.first].name;
}

const LocalisationDataEntry& LocOriginalData::getEntry(size_t idx) const
{
	const auto index = keyIndices.at(idx);
	return chunks[index.first].getEntry(index.second);
}

LocalisationDataEntry& LocOriginalData::getEntry(size_t idx)
{
	const auto index = keyIndices[idx];
	return chunks[index.first].getEntry(index.second);
}

LocalisationDataEntry* LocOriginalData::tryGetEntry(const String& key)
{
	const auto iter = keyMap.find(key);
	if (iter == keyMap.end()) {
		return nullptr;
	}
	return &getEntry(iter->second);
}

const LocalisationDataEntry* LocOriginalData::tryGetEntry(const String& key) const
{
	const auto iter = keyMap.find(key);
	if (iter == keyMap.end()) {
		return nullptr;
	}
	return &getEntry(iter->second);
}

bool LocOriginalData::setValue(const String& key, const String& value)
{
	if (auto* entry = tryGetEntry(key)) {
		entry->setValue(value);
		return true;
	}
	return false;
}

Vector<LocStringProperties> LocOriginalData::makeStringPropertiesDelta(const LocOriginalData& remote, const Vector<String>& keysModified) const
{
	Vector<LocStringProperties> result;

	for (const auto& key: keysModified) {
		const auto* mine = tryGetEntry(key);
		const auto* theirs = remote.tryGetEntry(key);

		if (mine && theirs) {
			auto entry = LocStringProperties(*theirs, *mine);
			if (entry.hasChange()) {
				result += std::move(entry);
			}
		}
	}

	return result;
}

void LocOriginalData::applyStringProperties(const Vector<LocStringProperties>& entries)
{
	for (const auto& change: entries) {
		if (auto* entry = tryGetEntry(change.key)) {
			change.apply(*entry);
		}
	}
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
			result.keyMap[entry.getKey()] = static_cast<int32_t>(result.keyIndices.size());
			result.keyIndices.emplace_back(result.chunks.size() - 1, i++);
		}
	}

	return result;
}

bool LocOriginalData::update(const LocOriginalData& other)
{
	bool modified = false;
	for (const auto& otherChunk: other.chunks) {
		for (const auto& otherEntry: otherChunk.entries) {
			if (auto* entry = tryGetEntry(otherEntry.getKey())) {
				*entry = otherEntry;
				modified = true;
			}
		}
	}
	return modified;
}

bool LocOriginalData::updateLocalFromRemote(const LocOriginalData& remote)
{
	// This method won't pull actual strings, as the local is considered authoritative
	// Only mark as modified if versions have changed

	bool modified = false;
	for (auto& chunk: chunks) {
		for (auto& entry: chunk.entries) {
			if (const auto* remoteEntry = remote.tryGetEntry(entry.getKey())) {
				if (entry.getVersion() != remoteEntry->getVersion()) {
					entry.setVersion(remoteEntry->getVersion());
					modified = true;
				}
				entry.setComment(remoteEntry->getComment());
				entry.setContext(remoteEntry->getContext());
				entry.setPriority(remoteEntry->getPriority());
			}
		}
	}

	Logger::logInfo("Remote has " + toString(remote.getNumEntries()) + " strs, local has " + getNumEntries());

	/*
	for (const auto& chunk: remote.getChunks()) {
		for (const auto& entry: chunk.entries) {
			if (!hasKey(entry.key)) {
				Logger::logInfo("Missing local key: " + entry.key + " [" + chunk.name + "]");
			}
		}
	}
	*/

	return modified;
}

void LocOriginalData::indexData()
{
	keyIndices.clear();
	keyMap.clear();

	const auto nChunks = chunks.size();
	for (size_t j = 0; j < nChunks; ++j) {
		const auto n = chunks[j].getNumEntries();
		for (size_t i = 0; i < n; ++i) {
			keyMap[chunks[j].getEntry(i).getKey()] = static_cast<int32_t>(keyIndices.size());
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
	: origVersion(origVersion)
	, value(std::move(value))
{
}

const String& LocTranslationEntry::getValue() const
{
	return value;
}

const String& LocTranslationEntry::getValueLowercase() const
{
	if (!valueLowercase) {
		valueLowercase = value.asciiLower();
	}
	return *valueLowercase;
}

void LocTranslationEntry::setValue(String value)
{
	this->value = std::move(value);
	valueLowercase = {};
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

bool LocTranslationData::setValue(const String& key, int32_t curVersion, String value)
{
	if (curVersion < 0) {
		throw Exception("Invalid current version for key", HalleyExceptions::Tools);
	}

	const auto iter = entries.find(key);
	if (iter == entries.end()) {
		// New key
		if (!value.isEmpty()) {
			entries[key] = LocTranslationEntry{ std::move(value), curVersion };
			return true;
		}
	} else {
		// Key exists
		if (iter->second.getValue() != value) {
			iter->second.setValue(value);
			iter->second.origVersion = curVersion;
			return true;
		}
	}

	return false;
}

bool LocTranslationData::setVersion(const String& key, int32_t curVersion)
{
	if (curVersion < 0) {
		return false;
	}

	const auto iter = entries.find(key);
	if (iter != entries.end()) {
		if (iter->second.origVersion != curVersion) {
			iter->second.origVersion = curVersion;
		}
		return true;
	}

	return false;
}

const LocTranslationEntry* LocTranslationData::tryGetEntry(const String& key) const
{
	const auto iter = entries.find(key);
	if (iter != entries.end()) {
		return &iter->second;
	}
	return nullptr;
}

TranslationStats LocTranslationData::getTranslationStats(const LocOriginalData& original, const LocalisationStats& origStats) const
{
	TranslationStats result;

	for (const auto& entry: entries) {
		if (auto* originalEntry = original.tryGetEntry(entry.first)) {
			const auto version = originalEntry->getVersion();
			const auto wordCount = origStats.wordsPerKey.value_or(entry.first, 0);

			if (version == entry.second.origVersion) {
				result.translatedKeys++;
				result.translatedWords += wordCount;
			} else {
				result.outdatedKeys++;
				result.outdatedWords += wordCount;
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
			result.entries[entry["key"].asString()] = LocTranslationEntry { entry["value"].asString(""), -1 };
		}
	}

	return result;
}

bool LocTranslationData::update(const LocTranslationData& other)
{
	int nModified = 0;
	for (const auto& [key, otherEntry]: other.entries) {
		auto iter = entries.find(key);
		if (iter == entries.end()) {
			iter = entries.insert_or_assign(key, LocTranslationEntry()).first;
		}
		auto& myEntry = iter->second;

		myEntry = otherEntry;
		++nModified;
	}
	return nModified > 0;
}

bool LocTranslationData::updateLocalFromRemote(const LocTranslationData& remote)
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

		if (remoteEntry.version > myEntry.version || myEntry.getValue().isEmpty()) {
			if (remoteEntry != myEntry) {
				myEntry = remoteEntry;
				++nModified;
			}
		}
	}

	if (nModified > 0) {
		Logger::logInfo("Updated " + toString(nModified) + " keys in " + language.getISOCode() + " from remote strings");
	}

	return nModified > 0;
}

void LocTranslationData::updateOriginalVersions(const LocOriginalData& originalLanguage)
{
	for (auto& [k, e]: entries) {
		if (e.origVersion == -1) {
			if (const auto* orig = originalLanguage.tryGetEntry(k)) {
				e.origVersion = orig->getVersion();
			}
		}
	}
}

bool LocTranslationData::pruneKeys(const LocOriginalData& originalLanguage)
{
	const auto nPruned = std_ex::erase_if_key(entries, [&] (const String& key) { return !originalLanguage.hasKey(key); });

	if (nPruned > 0) {
		Logger::logInfo("Pruned " + toString(nPruned) + " keys from " + language.getISOCode());
	}

	return nPruned > 0;
}

LocTranslationData LocTranslationData::makeDeltaFrom(const LocTranslationData& other) const
{
	assert(this->language == other.language);

	LocTranslationData result;
	result.language = language;

	for (const auto& [key, myEntry]: entries) {
		const auto* otherEntry = other.tryGetEntry(key);
		if (!otherEntry || myEntry.getValue() != otherEntry->getValue() || myEntry.origVersion != otherEntry->origVersion) {
			result.entries[key] = myEntry;
		}
	}

	return result;
}

LocTranslationData LocTranslationData::makeDeltaFrom(const LocTranslationData& other, gsl::span<const String> keys) const
{
	LocTranslationData result;
	result.language = language;

	for (const auto& key: keys) {
		if (const auto* myEntry = tryGetEntry(key)) {
			const auto* otherEntry = other.tryGetEntry(key);
			if (!otherEntry || myEntry->getValue() != otherEntry->getValue() || myEntry->origVersion != otherEntry->origVersion) {
				result.entries[key] = *myEntry;
			}
		}
	}

	return result;
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

bool LocStringSet::updateWith(const LocStringSet& other)
{
	bool modified = false;

	// Note: do NOT update version here! Partial updates need separate version tracking

	if (originalLanguage && other.originalLanguage) {
		modified = originalLanguage->update(*other.originalLanguage) || modified;
	}

	for (const auto& [k, v]: other.localised) {
		const auto iter = localised.find(k);
		if (iter != localised.end()) {
			modified = iter->second.update(v) || modified;
		}
	}

	return modified;
}

bool LocTranslationData::operator==(const LocTranslationData& other) const
{
	return language == other.language && entries == other.entries;
}

bool LocTranslationData::operator!=(const LocTranslationData& other) const
{
	return !(*this == other);
}

LocStringProperties::LocStringProperties(const LocalisationDataEntry& from, const LocalisationDataEntry& to)
{
	assert(from.key == to.key);

	key = from.getKey();
	if (from.getComment() != to.getComment()) {
		comment = to.getComment();
	}
	if (from.getContext() != to.getContext()) {
		context = to.getContext();
	}
	if (from.getPriority() != to.getPriority()) {
		priority = to.getPriority();
	}
}

void LocStringProperties::apply(LocalisationDataEntry& entry) const
{
	if (comment) {
		entry.setComment(*comment);
	}
	if (context) {
		entry.setContext(*context);
	}
	if (priority) {
		entry.setPriority(*priority);
	}
}

bool LocStringProperties::hasChange() const
{
	return comment || context || priority;
}

ConfigNode LocStringProperties::toConfigNode() const
{
	ConfigNode result;
	result["key"] = key;
	if (context) {
		result["context"] = context;
	}
	if (comment) {
		result["comment"] = comment;
	}
	if (priority) {
		result["priority"] = priority;
	}
	return result;
}
