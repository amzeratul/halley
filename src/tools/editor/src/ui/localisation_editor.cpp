#include "localisation_editor.h"

#include "halley/tools/project/project.h"
#include "halley/tools/project/project_properties.h"

using namespace Halley;

LocalisationEditor::LocalisationEditor(Project& project, UIFactory& factory)
	: UIWidget("localisation_editor", {}, UISizer())
    , project(project)
	, factory(factory)
	, originalLanguage(project.getProperties().getOriginalLanguage())
{
	setupCountryNames();
	setupLanguageNames();
}

void LocalisationEditor::update(Time t, bool moved)
{
	if (!loaded && project.isDLLLoaded()) {
		load();
	}
}

void LocalisationEditor::onMakeUI()
{
	populateData();
}

void LocalisationEditor::onActiveChanged(bool active)
{
	if (active && project.isDLLLoaded()) {
		populateData();
	}
}

void LocalisationEditor::onAssetsLoaded()
{
	if (isActiveInHierarchy() && project.isDLLLoaded()) {
		populateData();
	}
}

void LocalisationEditor::load()
{
	loaded = true;
	factory.loadUI(*this, "halley/localisation_editor");
	project.addAssetLoadedListener(this);
}

void LocalisationEditor::wordCount()
{
	totalCount = 0;
	totalKeys = 0;
	wordCounts.clear();
	localisedInfo.clear();

	HashSet<String> locKeys;
	const auto langs = project.getProperties().getLanguages();

	auto& gameResources = project.getGameResources();

	// Scan for original language
	for (auto& assetName: gameResources.enumerate<ConfigFile>()) {
		if (assetName.startsWith("strings/")) {
			auto& config = *gameResources.get<ConfigFile>(assetName);
			const String category = getCategory(assetName);

			for (auto& languageNode: config.getRoot().asMap()) {
				const auto language = I18NLanguage(languageNode.first);
				if (language == originalLanguage) {
					for (auto& e: languageNode.second.asMap()) {
						const auto& str = e.second.asString();

						if (!locKeys.contains(e.first)) {
							locKeys.insert(e.first);
							++totalKeys;

							const auto curCount = getWordCount(str);
							wordCounts[category] += curCount;
							totalCount += curCount;
						} else {
							Logger::logWarning("Duplicated localisation key \"" + e.first + "\" found in file " + assetName);
						}
					}
				}
			}
		}
	}

	// Scan for localisation
	for (auto& assetName: gameResources.enumerate<ConfigFile>()) {
		if (assetName.startsWith("strings/")) {
			auto& config = *gameResources.get<ConfigFile>(assetName);

			for (auto& languageNode: config.getRoot().asMap()) {
				const auto language = I18NLanguage(languageNode.first);

				if (language != originalLanguage && std_ex::contains(langs, language)) {
					const auto langCode = language.getISOCode();
					for (auto& e: languageNode.second.asMap()) {
						if (locKeys.contains(e.first)) {
							++localisedInfo[langCode].keysTranslated;
						}
					}
				}
			}
		}
	}
}

int LocalisationEditor::getWordCount(const String& line)
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

String LocalisationEditor::getCategory(const String& assetId) const
{
	if (project.getGameInstance()) {
		return project.getGameInstance()->getLocalisationFileCategory(assetId);
	} else {
		return "unknown";
	}
}

String LocalisationEditor::getLanguageName(const I18NLanguage& ln) const
{
	auto cc = ln.getCountryCode();
	auto lc = ln.getLanguageCode();
	String country, language;

	if (const auto iter = languageNames.find(lc); iter != languageNames.end()) {
		language = iter->second;
	} else {
		language = lc;
	}
	
	if (cc && languageNeedsQualifier.contains(lc)) {
		if (const auto iter = countryNames.find(*cc); iter != countryNames.end()) {
			country = iter->second;
		} else {
			country = *cc;
		}
	}

	return country.isEmpty() ? language : language + " (" + country + ")";
}

Sprite LocalisationEditor::getFlag(const I18NLanguage& language) const
{
	if (language.getCountryCode()) {
		auto countryCode = language.getCountryCode()->asciiLower();
		if (countryCode == "hans") {
			countryCode = "cn";
		} else if (countryCode == "hant") {
			countryCode = "hk";
		}

		return Sprite().setImage(factory.getResources(), "flags/h20/" + countryCode + ".png");
	} else {
		return {};
	}
}

String LocalisationEditor::getNumberWithCommas(int number) const
{
	if (number >= 1'000'000) {
		return toString(number / 1'000'000) + "," + toString((number % 1000000) / 1000, 10, 3) + "," + toString(number % 1000, 10, 3);
	} else if (number >= 1000) {
		return toString(number / 1000) + "," + toString(number % 1000, 10, 3);
	} else {
		return toString(number);
	}
}

void LocalisationEditor::populateData()
{
	if (!loaded) {
		if (project.isDLLLoaded()) {
			load();
		} else {
			return;
		}
	}
	wordCount();

	getWidgetAs<UIImage>("mainLanguageFlag")->setSprite(getFlag(originalLanguage));
	getWidgetAs<UILabel>("mainLanguage")->setText(LocalisedString::fromUserString(getLanguageName(originalLanguage)));
	getWidgetAs<UILabel>("wordCount")->setText(LocalisedString::fromUserString(getNumberWithCommas(totalCount)));

	auto byCategory = getWidget("byCategory");
	byCategory->clear();

	auto labelStyle = factory.getStyle("label");
	for (const auto& [k, v]: wordCounts) {
		byCategory->add(std::make_shared<UILabel>("", labelStyle, LocalisedString::fromUserString(k + ":")));
		byCategory->add(std::make_shared<UILabel>("", labelStyle, LocalisedString::fromUserString(getNumberWithCommas(v))));
	}

	auto languagesContainer = getWidget("languages");
	languagesContainer->clear();

	for (const auto& lang: project.getProperties().getLanguages()) {
		if (lang != originalLanguage) {
			addTranslationData(*languagesContainer, lang);
		}
	}
}

void LocalisationEditor::addTranslationData(UIWidget& container, const I18NLanguage& language)
{
	auto widget = factory.makeUI("halley/localisation_language_summary");
	widget->layout();

	widget->getWidgetAs<UIImage>("flag")->setSprite(getFlag(language));
	widget->getWidgetAs<UILabel>("languageName")->setText(LocalisedString::fromUserString(getLanguageName(language)));

	const auto iter = localisedInfo.find(language.getISOCode());
	const auto info = iter == localisedInfo.end() ? LocalisationInfo{} : iter->second;

	const int translatedPercent = std::max((info.keysTranslated * 100) / totalKeys, info.keysTranslated > 0 ? 1 : 0);

	const auto rect = Rect4i(widget->getWidget("bar_full")->getRect());
	const int totalW = rect.getWidth() - 2;
	const int totalH = rect.getHeight();
	const int greenW = std::max((info.keysTranslated * totalW) / totalKeys, info.keysTranslated > 0 ? 1 : 0);
	const int yellowW = std::max((info.keysOutdated * totalW) / totalKeys, info.keysOutdated > 0 ? 1 : 0);

	widget->getWidgetAs<UILabel>("completion")->setText(LocalisedString::fromUserString(toString(translatedPercent) + "% complete"));
	widget->getWidgetAs<UIImage>("bar_green")->setLocalClip(Rect4f(Rect4i(0, 0, greenW, totalH)));
	widget->getWidgetAs<UIImage>("bar_yellow")->setLocalClip(Rect4f(Rect4i(greenW, 0, yellowW, totalH)));

	container.add(widget);
}

void LocalisationEditor::setupCountryNames()
{
	countryNames["GB"] = "United Kingdom";
	countryNames["US"] = "United States";
	countryNames["CA"] = "Canada";
	countryNames["FR"] = "France";
	countryNames["ES"] = "Spain";
	countryNames["IT"] = "Italy";
	countryNames["PT"] = "Portugal";
	countryNames["BR"] = "Brazil";
	countryNames["DE"] = "Germany";
	countryNames["RU"] = "Russia";
	countryNames["JP"] = "Japan";
	countryNames["Hans"] = "Simplified";
	countryNames["Hant"] = "Traditional";
	countryNames["KR"] = "South Korea";
	countryNames["TH"] = "Thailand";
	countryNames["PL"] = "Poland";
	countryNames["TR"] = u8"Türkiye";
	countryNames["UA"] = "Ukraine";
	countryNames["AG"] = "Argentina";
	countryNames["MX"] = "Mexico";
	countryNames["CL"] = "Chile";
}

void LocalisationEditor::setupLanguageNames()
{
	languageNames["en"] = "English";
	languageNames["fr"] = "French";
	languageNames["es"] = "Spanish";
	languageNames["it"] = "Italian";
	languageNames["pt"] = "Portuguese";
	languageNames["de"] = "German";
	languageNames["ru"] = "Russian";
	languageNames["ja"] = "Japanese";
	languageNames["zh"] = "Chinese";
	languageNames["ko"] = "Korean";
	languageNames["th"] = "Thai";
	languageNames["pl"] = "Polish";
	languageNames["tr"] = "Turkish";
	languageNames["uk"] = "Ukrainian";

	languageNeedsQualifier.insert("en");
	languageNeedsQualifier.insert("pt");
	languageNeedsQualifier.insert("zh");
	languageNeedsQualifier.insert("fr");
	languageNeedsQualifier.insert("es");
}
