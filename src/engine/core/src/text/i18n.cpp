#include <utility>
#include "halley/text/i18n.h"
#include "halley/file_formats/config_file.h"

using namespace Halley;

I18N::I18N()
{
}

I18N::I18N(Resources& resources, I18NLanguage currentLanguage, std::optional<I18NLanguage> fallbackLanguage)
{
	loadStrings(resources);
	setCurrentLanguage(currentLanguage);
	if (fallbackLanguage) {
		setFallbackLanguage(*fallbackLanguage);
	}
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
		}
	}
}

void I18N::loadLocalisationFile(const ConfigFile& config)
{
	loadLocalisation(config.getRoot(), config.getAssetId(), false);
#ifdef DEV_BUILD
	observers[config.getAssetId()] = ConfigObserver(config);
#endif
}

void I18N::loadLocalisation(const ConfigNode& root, const String& assetId, bool allowUpdating)
{
	for (auto& language: root.asMap()) {
		auto langCode = I18NLanguage(language.first);
		auto& lang = strings[langCode];
		for (auto& e: language.second.asMap()) {
			if (e.first == "null") {
				Logger::logWarning("null key found on localisation file " + assetId);
				continue;
			}

			if (!allowUpdating) {
				if (const auto iter = lang.find(e.first); iter != lang.end()) {
					Logger::logError("Duplicated localisation key \"" + e.first + "\": Previously set to \"" + iter->second + "\", now \"" + e.second.asString() + "\" in " + assetId);
					continue;
				}
			}
			lang[e.first] = e.second.asString();
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

void I18N::setCurrentLanguage(const I18NLanguage& code)
{
	currentLanguage = code;
	++version;
}

void I18N::setFallbackLanguage(const I18NLanguage& code)
{
	fallbackLanguage = code;
}

Vector<I18NLanguage> I18N::getLanguagesAvailable() const
{
	Vector<I18NLanguage> result;
	for (auto& e: strings) {
		result.push_back(e.first);
	}
	return result;
}

LocalisedString I18N::get(const String& key) const
{
	if (auto str = tryGet(key)) {
		return *str;
	}

#ifdef DEV_BUILD
	return LocalisedString(*this, key, "#MISSING:" + key + "#");
#else
	return LocalisedString(*this, key, "#MISSING#");
#endif
}

std::optional<LocalisedString> I18N::tryGet(const String& key) const
{
	if (key.isEmpty()) {
		return std::nullopt;
	}

	auto curLang = strings.find(currentLanguage);
	if (curLang != strings.end()) {
		auto i = curLang->second.find(key);
		if (i != curLang->second.end()) {
			return LocalisedString(*this, key, i->second);
		}
	}

	if (fallbackLanguage && fallbackLanguage.value() != currentLanguage) {
		auto defLang = strings.find(fallbackLanguage.value());
		if (defLang != strings.end()) {
			auto i = defLang->second.find(key);
			if (i != defLang->second.end()) {
				return LocalisedString(*this, key, i->second);
			}
		}
	}

	return std::nullopt;
}

std::optional<LocalisedString> I18N::get(const String& key, const I18NLanguage& language) const
{
	auto curLang = strings.find(language);
	if (curLang != strings.end()) {
		auto i = curLang->second.find(key);
		if (i != curLang->second.end()) {
			return LocalisedString(*this, key, i->second);
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

const I18NLanguage& I18N::getCurrentLanguage() const
{
	return currentLanguage;
}

int I18N::getVersion() const
{
	return version;
}

char I18N::getDecimalSeparator() const
{
	return currentLanguage.getDecimalSeparator();
}

void I18N::checkForDuplicatedStrings(const Vector<String>& ignoredPrefixes) const
{
	HashMap<String, Vector<String>> strToKeys;

	const auto& strs = strings.at(currentLanguage);
	for (const auto& [k, v]: strs) {
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
	std::sort(sortedResults.begin(), sortedResults.end(), [&] (const auto& a, const auto& b) {
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

LocalisedString::LocalisedString()
{
}

LocalisedString& LocalisedString::operator+=(const LocalisedString& str)
{
	string += str.string;
	return *this;
}

LocalisedString::LocalisedString(String string)
	: string(std::move(string))
{
}

LocalisedString::LocalisedString(const I18N& i18n, String key, String string)
	: i18n(&i18n)
	, key(std::move(key))
	, string(std::move(string))
	, i18nVersion(i18n.getVersion())
{
}

I18NLanguage::I18NLanguage()
{
}

I18NLanguage::I18NLanguage(const String& code)
{
	if (code.contains("-")) {
		auto split = code.split('-');
		set(split.at(0), split.at(1));
	} else if (code.contains("_")) {
		auto split = code.split('_');
		set(split.at(0), split.at(1));
	} else {
		set(code, {});
	}
}

I18NLanguage::I18NLanguage(String languageCode, std::optional<String> countryCode)
{
	set(std::move(languageCode), std::move(countryCode));
}

I18NLanguage::I18NLanguage(const ConfigNode& node)
{
	*this = I18NLanguage(node.asString(""));
}

ConfigNode I18NLanguage::toConfigNode() const
{
	return ConfigNode(getISOCode());
}

void I18NLanguage::set(String languageCode, std::optional<String> countryCode)
{
	this->languageCode = std::move(languageCode);
	this->countryCode = std::move(countryCode);
}

const String& I18NLanguage::getLanguageCode() const
{
	return languageCode;
}

const std::optional<String>& I18NLanguage::getCountryCode() const
{
	return countryCode;
}

String I18NLanguage::getISOCode() const
{
	if (countryCode) {
		return languageCode + "-" + countryCode.value();
	} else {
		return languageCode;
	}
}

char I18NLanguage::getDecimalSeparator() const
{
	if (languageCode == "fr" || languageCode == "pt" || languageCode == "es" || languageCode == "it" || languageCode == "de") {
		return ',';
	}
	return '.';
}

I18NLanguageMatch I18NLanguage::getMatch(const I18NLanguage& other) const
{
	if (languageCode != other.languageCode) {
		return I18NLanguageMatch::None;
	}
	if (countryCode != other.countryCode) {
		return I18NLanguageMatch::Good;
	}
	return I18NLanguageMatch::Exact;
}

std::optional<I18NLanguage> I18NLanguage::getBestMatch(const Vector<I18NLanguage>& languages, const I18NLanguage& target, std::optional<I18NLanguage> fallback)
{
	I18NLanguageMatch bestMatch = I18NLanguageMatch::None;
	std::optional<I18NLanguage> result = fallback;
	for (const auto& l: languages) {
		auto m = l.getMatch(target);
		if (int(m) > int(bestMatch)) {
			bestMatch = m;
			result = l;
		}
	}
	return result;
}

bool I18NLanguage::operator==(const I18NLanguage& other) const
{
	return languageCode == other.languageCode && countryCode == other.countryCode;
}

bool I18NLanguage::operator!=(const I18NLanguage& other) const
{
	return languageCode != other.languageCode || countryCode != other.countryCode;
}

bool I18NLanguage::operator<(const I18NLanguage& other) const
{
	if (languageCode != other.languageCode) {
		return languageCode < other.languageCode;
	}
	if (countryCode == other.countryCode) {
		return false;
	}
	if (!countryCode) {
		return true;
	}
	if (!other.countryCode) {
		return false;
	}
	return countryCode.value() < other.countryCode.value();
}

LocalisedString LocalisedString::fromHardcodedString(const char* str)
{
	return LocalisedString(String(str));
}

LocalisedString LocalisedString::fromHardcodedString(const String& str)
{
	return LocalisedString(String(str));
}

LocalisedString LocalisedString::fromUserString(const String& str)
{
	return LocalisedString(str);
}

LocalisedString LocalisedString::fromNumber(int number, int base, int width, char fill)
{
	return LocalisedString(Halley::toString(number, base, width, fill));
}

LocalisedString LocalisedString::fromNumber(float number, const I18NLanguage& language, int precisionDigits, bool fixed)
{
	return LocalisedString(Halley::toString(number, precisionDigits, language.getDecimalSeparator(), fixed));
}

LocalisedString LocalisedString::replaceTokens(const LocalisedString& tok0) const
{
	return LocalisedString(string.replaceAll("{0}", tok0.getString()));
}

LocalisedString LocalisedString::replaceTokens(const LocalisedString& tok0, const LocalisedString& tok1) const
{
	return LocalisedString(string.replaceAll("{0}", tok0.getString()).replaceAll("{1}", tok1.getString()));
}

LocalisedString LocalisedString::replaceTokens(const LocalisedString& tok0, const LocalisedString& tok1, const LocalisedString& tok2) const
{
	return LocalisedString(string.replaceAll("{0}", tok0.getString()).replaceAll("{1}", tok1.getString()).replaceAll("{2}", tok2.getString()));
}

LocalisedString LocalisedString::replaceTokens(const LocalisedString& tok0, const LocalisedString& tok1, const LocalisedString& tok2, const LocalisedString& tok3) const
{
	return LocalisedString(string.replaceAll("{0}", tok0.getString()).replaceAll("{1}", tok1.getString()).replaceAll("{2}", tok2.getString()).replaceAll("{3}", tok3.getString()));
}

LocalisedString LocalisedString::replaceTokens(gsl::span<const LocalisedString> toks) const
{
	if (toks.empty()) {
		return *this;
	}
	auto str = string;
	for (int i = 0; i < int(toks.size()); ++i) {
		str = str.replaceAll("{" + Halley::toString(i) + "}", toks[i].getString());
	}
	return LocalisedString(str);
}

std::pair<LocalisedString, Vector<ColourOverride>> LocalisedString::replaceTokens(gsl::span<const LocalisedString> toks, gsl::span<const std::optional<Colour4f>> colours) const
{
	assert(toks.size() == colours.size());
	if (toks.empty()) {
		return { *this, {} };
	}

	Vector<std::pair<int, size_t>> indices;

	for (int i = 0; i < int(toks.size()); ++i) {
		const auto pos = string.find("{" + Halley::toString(i) + "}");
		if (pos != String::npos) {
			indices.emplace_back(i, pos);
		}
	}

	std::sort(indices.begin(), indices.end(), [] (const auto& a, const auto& b) { return a.second < b.second; });

	auto str = std::string_view(string);

	size_t lastPos = 0;
	ColourStringBuilder builder;
	for (const auto& index: indices) {
		builder.append(str.substr(lastPos, index.second - lastPos));
		builder.append(toks[index.first].getString(), colours[index.first]);
		lastPos = index.second + 3;
	}
	builder.append(str.substr(lastPos));

	auto result = builder.moveResults();
	return { LocalisedString(std::move(result.first)), std::move(result.second) };
}

LocalisedString LocalisedString::replaceTokens(const std::map<String, LocalisedString>& tokens) const
{
	auto curString = string;
	int idx = 0;
	for (const auto& token : tokens) {
		curString = string.replaceAll("{" + token.first + "}", token.second.getString());
		++idx;
	}
	return LocalisedString(curString);
}

LocalisedString LocalisedString::replaceToken(const String& pattern, const LocalisedString& token) const
{
	return fromUserString(string.replaceAll(pattern, token.getString()));
}

const String& LocalisedString::getString() const
{
	return string;
}

const String& LocalisedString::toString() const
{
	return string;
}

bool LocalisedString::operator==(const LocalisedString& other) const
{
	return string == other.string;
}

bool LocalisedString::operator!=(const LocalisedString& other) const
{
	return string != other.string;
}

bool LocalisedString::operator<(const LocalisedString& other) const
{
	return string < other.string;
}

LocalisedString LocalisedString::operator+(const LocalisedString& other) const
{
	return LocalisedString(string + other.string);
}

bool LocalisedString::checkForUpdates()
{
#ifdef DEV_BUILD
	if (i18n) {
		const auto curVersion = i18n->getVersion();
		if (i18nVersion != curVersion) {
			const auto newValue = i18n->get(key);
			i18nVersion = curVersion;
			if (string != newValue.string) {
				string = newValue.string;
				return true;
			}
		}
	}
#endif
	return false;
}
