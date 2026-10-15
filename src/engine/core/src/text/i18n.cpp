#include <utility>
#include "halley/text/i18n.h"

#include "halley/api/halley_api.h"
#include "halley/file_formats/config_file.h"
#include "halley/resources/resources.h"
#include "halley/text/localised_string.h"
#include "halley/text/string_output_server.h"
#include "halley/utils/algorithm.h"
#include "halley/utils/memory_usage_calculator.h"

using namespace Halley;

namespace {
#if defined(DEV_BUILD)
	constexpr bool canHotReload = isPCPlatform();
#else
	constexpr bool canHotReload = false;
#endif
}

I18N::I18N()
{
}

I18N::I18N(Resources& resources, I18NLanguage currentLanguage, std::optional<I18NLanguage> fallbackLanguage)
	: resources(&resources)
{
	setCurrentLanguage(currentLanguage);
	if (fallbackLanguage) {
		setFallbackLanguage(*fallbackLanguage);
	}
	loadStrings(resources);
}

I18N::~I18N()
{
	
}

void I18N::update()
{
	for (auto& o: observers) {
		if (o.second.needsUpdate()) {
			o.second.update();
			loadLocalisation(o.second.getRoot(), o.first, true);
		}
	}
}

void I18N::loadStrings(Resources& resources)
{
	for (auto& assetName : resources.enumerate<ConfigFile>()) {
		if (assetName.startsWith("strings/")) {
			//resources.of<ConfigFile>().unload(assetName);
			loadLocalisationFile(*resources.get<ConfigFile>(assetName));
			if constexpr (!canHotReload) {
				resources.unload<ConfigFile>(assetName);
			}
		}
	}
}

void I18N::loadLocalisationFile(const ConfigFile& config)
{
	loadLocalisation(config.getRoot(), config.getAssetId(), false);
	if constexpr (canHotReload) {
		observers[config.getAssetId()] = ConfigObserver(config);
	}
}

I18N::LangData& I18N::getLanguageData(const I18NLanguage& language)
{
	if (const auto iter = strings.find(language); iter != strings.end()) {
		return iter->second;
	}
	strings[language] = LangData{ {}, static_cast<int>(languageIndices.size()) };
	languageIndices += language;
	return strings.at(language);
}

void I18N::loadLocalisation(const ConfigNode& root, const String& assetId, bool allowUpdating)
{
	// Make sure this is index 0
	getLanguageData(fallbackLanguage.value_or(currentLanguage));

	if (root.getType() == ConfigNodeType::Map) {
		for (auto& language: root.asMap()) {
			auto langCode = I18NLanguage(language.first);
			auto& lang = getLanguageData(langCode);
			if (!lang.assetIds.contains(assetId)) {
				lang.assetIds += assetId;
			}

			if (shouldBeLoaded(langCode)) {
				doLoadLocalisation(langCode, language.second, assetId, allowUpdating);
				lang.loaded = true;
			}
		}
	}
	++version;
}

void I18N::doLoadLocalisation(const I18NLanguage& langCode, const ConfigNode& node, const String& assetId, bool allowUpdating)
{
	auto& lang = getLanguageData(langCode);
	for (auto& e: node.asMap()) {
		if (e.first == "null") {
			Logger::logWarning("null key found on localisation file " + assetId);
			continue;
		}

		if (!allowUpdating) {
			if (const auto iter = lang.strings.find(e.first); iter != lang.strings.end()) {
				Logger::logError("Duplicated localisation key \"" + e.first + "\": Previously set to \"" + iter->second + "\", now \"" + e.second.asString() + "\" in " + assetId);
				continue;
			}
		}
		lang.strings[e.first] = e.second.asString();
	}
}

void I18N::loadLanguage(const I18NLanguage& language)
{
	auto& lang = getLanguageData(language);
	lang.strings.clear();
	for (auto& assetId: lang.assetIds) {
		const auto& config = resources->get<ConfigFile>(assetId);
		doLoadLocalisation(language, config->getRoot().asMap().at(language.getISOCode()), assetId, false);
		if constexpr (!canHotReload) {
			resources->unload<ConfigFile>(assetId);
		}
	}
	lang.loaded = true;
}

void I18N::unloadLanguage(const I18NLanguage& language)
{
	auto& lang = getLanguageData(language);
	lang.strings.clear();
	lang.strings.shrink_to_fit();
	lang.loaded = false;
}

void I18N::updateLoadedLanguages()
{
	for (auto& [langId, langData]: strings) {
		const bool target = shouldBeLoaded(langId);
		if (target != langData.loaded) {
			if (target) {
				loadLanguage(langId);
				//Logger::logInfo("Loaded " + langId.getISOCode());
			} else {
				unloadLanguage(langId);
				//Logger::logInfo("Unloaded " + langId.getISOCode());
			}
		}
	}
	//Logger::logDev("I18N memory usage: " + String::prettySize(getMemoryUsage()));
	//Logger::logDev("Known languages: " + toString(strings.size()));
}

bool I18N::shouldBeLoaded(const I18NLanguage& language) const
{
	return canHotReload || language == currentLanguage || language == secondaryLanguage || language == fallbackLanguage || language == tempOverrideLanguage;
}

void I18N::updateStrings(const I18NLanguage& language, HashMap<String, String> newStrings)
{
	if (newStrings.empty()) {
		return;
	}

	auto& lang = getLanguageData(language);
	for (auto& [k, v]: newStrings) {
		if (v.isEmpty()) {
			lang.strings.erase(k);
		} else {
			lang.strings[k] = std::move(v);
		}
	}
	++version;
}

I18NVersionChecker::I18NVersionChecker(const I18N& i18n)
{
	setI18N(i18n);
}

void I18NVersionChecker::setI18N(const I18N& i18n)
{
	this->i18n = &i18n;
	version = i18n.getVersion();
}

bool I18NVersionChecker::checkChanged()
{
	if (!i18n) {
		return false;
	}

	const auto curVersion = i18n->getVersion();
	if (version != curVersion) {
		version = curVersion;
		return true;
	}

	return false;
}

void I18N::setCurrentLanguage(I18NLanguage language)
{
	if (language != currentLanguage) {
		Logger::logInfo("Setting language: " + language.getISOCode());
		currentLanguage = std::move(language);
		++version;
		updateLoadedLanguages();
	}
}

const I18NLanguage& I18N::getCurrentLanguage() const
{
	if (tempOverrideLanguage) {
		return *tempOverrideLanguage;
	}
	return currentLanguage;
}

const I18NLanguage& I18N::getChosenCurrentLanguage() const
{
	return currentLanguage;
}

void I18N::setFallbackLanguage(std::optional<I18NLanguage> language)
{
	if (language != fallbackLanguage) {
		fallbackLanguage = std::move(language);
		updateLoadedLanguages();
	}
}

const std::optional<I18NLanguage>& I18N::getFallbackLanguage() const
{
	return fallbackLanguage;
}

void I18N::setSecondaryLanguage(std::optional<I18NLanguage> language)
{
	if (secondaryLanguage != language) {
		secondaryLanguage = std::move(language);
		updateLoadedLanguages();
	}
}

const std::optional<I18NLanguage>& I18N::getSecondaryLanguage() const
{
	return secondaryLanguage;
}

void I18N::setTempOverrideLanguage(std::optional<I18NLanguage> language)
{
	if (language != tempOverrideLanguage) {
		tempOverrideLanguage = std::move(language);
		++version;
		updateLoadedLanguages();
	}
}

const std::optional<I18NLanguage>& I18N::getTempOverrideLanguage() const
{
	return tempOverrideLanguage;
}

Vector<I18NLanguage> I18N::getLanguagesAvailable(std::optional<I18NLanguage> idealLanguage) const
{
	std::optional<size_t> bestIdx;
	I18NLanguageMatch bestMatch = I18NLanguageMatch::None;

	Vector<I18NLanguage> result;
	for (const auto& [lang, str]: strings) {
		const auto match = idealLanguage ? lang.getMatch(*idealLanguage) : I18NLanguageMatch::None;
		if (match > bestMatch) {
			bestMatch = match;
			bestIdx = result.size();
		} else if (!bestIdx && lang.getLanguageCode() == "en") {
			bestIdx = result.size();
		}

		result.push_back(lang);
	}

	std::sort(result.begin(), result.end());

	if (bestIdx) {
		const auto lang = result[*bestIdx];
		result.erase(result.begin() + *bestIdx);
		result.insert(result.begin(), lang);
	}

	return result;
}

LocalisedString I18N::get(const String& key) const
{
	if (auto str = tryGet(key)) {
		return *str;
	}

#ifdef DEV_BUILD
	return LocalisedString(*this, key, "#MISSING:" + key + "#", 0);
#else
	return LocalisedString(*this, key, "#MISSING#", 0);
#endif
}

std::optional<LocalisedString> I18N::tryGet(const String& key) const
{
	if (key.isEmpty()) {
		return std::nullopt;
	}

	const auto& activeLanguage = getCurrentLanguage();

	auto curLang = strings.find(activeLanguage);
	if (curLang != strings.end()) {
		auto i = curLang->second.strings.find(key);
		if (i != curLang->second.strings.end()) {
			return LocalisedString(*this, key, i->second, curLang->second.index);
		}
	}

	if (fallbackLanguage && fallbackLanguage.value() != activeLanguage) {
		auto defLang = strings.find(fallbackLanguage.value());
		if (defLang != strings.end()) {
			auto i = defLang->second.strings.find(key);
			if (i != defLang->second.strings.end()) {
				return LocalisedString(*this, key, i->second, defLang->second.index);
			}
		}
	}

	return std::nullopt;
}

LocalisedString I18N::get(const String& key, const I18NLanguage& language) const
{
	if (auto str = tryGet(key, language)) {
		return *str;
	}

#ifdef DEV_BUILD
	return LocalisedString(*this, key, "#MISSING:" + key + "#", getLanguageIndex(language));
#else
	return LocalisedString(*this, key, "#MISSING#", getLanguageIndex(language));
#endif
}

std::optional<LocalisedString> I18N::tryGet(const String& key, const I18NLanguage& language) const
{
	auto curLang = strings.find(language);
	if (curLang != strings.end()) {
		auto i = curLang->second.strings.find(key);
		if (i != curLang->second.strings.end()) {
			return LocalisedString(*this, key, i->second, curLang->second.index);
		}
	}

	return {};
}

LocalisedString I18N::getPreProcessedUserString(const String& string) const
{
	if (string.startsWith("$")) {
		return get(string.mid(1));
	} else {
		return LocalisedString::fromUserString(string);
	}
}

const I18NLanguage& I18N::getLanguageFromIndex(int languageIdx) const
{
	if (languageIdx < 0 || languageIdx >= static_cast<int>(languageIndices.size())) {
		return languageIndices.front();
	}
	return languageIndices[languageIdx];
}

int I18N::getLanguageIndex(const I18NLanguage& language) const
{
	if (const auto iter = strings.find(language); iter != strings.end()) {
		return iter->second.index;
	}
	return 0;
}

int I18N::getVersion() const
{
	return version;
}

char I18N::getDecimalSeparator() const
{
	return getCurrentLanguage().getDecimalSeparator();
}

void I18N::checkForDuplicatedStrings(const Vector<String>& ignoredPrefixes) const
{
	HashMap<String, Vector<String>> strToKeys;

	const auto& strs = strings.at(currentLanguage);
	for (const auto& [k, v]: strs.strings) {
		if (!k.startsWithAnyOf(ignoredPrefixes)) {
			strToKeys[v.toString()] += k;
		}
	}

	Vector<std::pair<String, Vector<String>>> sortedResults;

	for (const auto& [str, keys]: strToKeys) {
		if (keys.size() > 1) {
			sortedResults += std::pair(str, keys);
			auto& ss = sortedResults.back().second;
			std::sort(ss.begin(), ss.end());
		}
	}
	std::stable_sort(sortedResults.begin(), sortedResults.end(), [&] (const auto& a, const auto& b) {
		return a.second.front() < b.second.front();
	});

	if (sortedResults.empty()) {
		Logger::logDev("No duplicated strings found");
	} else {
		Logger::logDev("Found " + toString(sortedResults.size()) + " sets of duplicated strings:");
		for (const auto& [str, keys]: sortedResults) {
			Logger::logDev("* " + toString(keys.size()) + " duplicated keys: [" + String::concatList(keys, ", ") + "]: \"" + str + "\"");
		}
	}
}

Vector<uint32_t> I18N::getCodepointsUsedBy(const I18NLanguage& language) const
{
	std::set<uint32_t> characters;

	if (const auto iter = strings.find(language); iter != strings.end()) {
		for (const auto& [key, string]: iter->second.strings) {
			for (auto c: string.getUTF32()) {
				characters.insert(c);
			}
		}
	} else {
		Logger::logError("Language not found: " + language.getISOCode());
	}

	Vector<uint32_t> result;
	result.reserve(characters.size());
	for (auto c: characters) {
		result += c;
	}

	return result;
}

void I18N::checkForCodepointsInFonts(gsl::span<const std::shared_ptr<const Font>> fonts) const
{
	HashMap<String, Vector<uint32_t>> codepointsPerLanguage;
	for (auto& language: getLanguagesAvailable()) {
		auto codepoints = getCodepointsUsedBy(language);
		Logger::logInfo("[" + language.getISOCode() + "] " + toString(codepoints.size()) + " unique codepoints");
		codepointsPerLanguage[language.getISOCode()] = std::move(codepoints);
	}

	for (const auto& font: fonts) {

		for (const auto& [language, codepoints]: codepointsPerLanguage) {
			String missingList;
			String missingDetails;
			for (const auto& codepoint: codepoints) {
				if (!font->tryGetFontForGlyph(static_cast<int>(codepoint))) {
					missingList += String(static_cast<int>(codepoint));
					missingDetails += "\n  " + String(static_cast<int>(codepoint)) + " [U+" + toString(static_cast<int>(codepoint), 16, 4) + "]";
				}
			}

			if (missingList.isEmpty()) {
				Logger::logInfo("[" + font->getAssetId() + "] [" + language + "] OK (" + codepoints.size() + " characters)");
			} else {
				Logger::logInfo("[" + font->getAssetId() + "] [" + language + "] Missing characters: " + missingList + missingDetails);
			}
		}
	}
}

bool I18N::createStringOutputServer(const HalleyAPI& api, const String& host, int port)
{
	stringOutputServer = std::make_unique<StringOutputServer>(*this);
	if (api.webServer) {
		return stringOutputServer->startServer(api.webServer, host, port);
	} else {
		return false;
	}
}

StringOutputServer* I18N::tryGetStringOutputServer() const
{
	return stringOutputServer.get();
}

I18NLanguage I18N::getNextLanguageAvailable(const I18NLanguage& after) const
{
	auto langs = getLanguagesAvailable();
	if (langs.empty()) {
		return after;
	}

	std::sort(langs.begin(), langs.end());

	if (std::optional<size_t> idx = std_ex::find_index(langs, after)) {
		return langs.at((*idx + 1) % langs.size());
	} else {
		return langs.front();
	}
}

void I18N::setCurrentLanguageToNextOneAvailable()
{
	setCurrentLanguage(getNextLanguageAvailable(getChosenCurrentLanguage()));
}

size_t I18N::getMemoryUsage() const
{
	return getSizeBytesOf(strings);
}

size_t I18N::LangData::getSizeBytes() const
{
	return sizeof(this) + getSizeBytesOf(strings);
}
