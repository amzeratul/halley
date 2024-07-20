#include "localisation_editor.h"

#include "halley/tools/project/project.h"
#include "halley/tools/project/project_properties.h"

using namespace Halley;

LocalisationEditor::LocalisationEditor(Project& project, UIFactory& factory)
	: UIWidget("localisation_editor", {}, UISizer())
    , project(project)
	, factory(factory)
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

void LocalisationEditor::loadFromResources()
{
	// Scan for original language
	originalLanguage = LocalisationData::generateFromProject(project.getProperties().getOriginalLanguage(), project, *this);

	// Scan for localisation from HDD
	localised.clear();
	for (const auto& lang: project.getProperties().getLanguages()) {
		localised[lang.getISOCode()] = LocalisationData::generateFromProject(lang, project, *this);
	}
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
	loadFromResources();

	const auto origStats = originalLanguage.getStats();
	getWidgetAs<UIImage>("mainLanguageFlag")->setSprite(getFlag(originalLanguage.language));
	getWidgetAs<UILabel>("mainLanguage")->setText(LocalisedString::fromUserString(getLanguageName(originalLanguage.language)));
	getWidgetAs<UILabel>("wordCount")->setText(LocalisedString::fromUserString(getNumberWithCommas(origStats.totalWords)));
	getWidgetAs<UILabel>("keyCount")->setText(LocalisedString::fromUserString(getNumberWithCommas(origStats.totalKeys)));

	auto labelStyle = factory.getStyle("label");
	auto labelLightStyle = factory.getStyle("labelLight");
	auto byCategory = getWidget("byCategory");
	byCategory->clear();
	byCategory->add(std::make_shared<UILabel>("", labelStyle, LocalisedString::fromHardcodedString("Category")));
	byCategory->add(std::make_shared<UILabel>("", labelStyle, LocalisedString::fromHardcodedString("Words")));
	byCategory->add(std::make_shared<UILabel>("", labelStyle, LocalisedString::fromHardcodedString("Keys")));
	byCategory->add(std::make_shared<UILabel>("", labelStyle, LocalisedString::fromHardcodedString("Words/Key")));

	for (const auto& [k, v]: origStats.wordsPerCategory) {
		byCategory->add(std::make_shared<UILabel>("", labelLightStyle, LocalisedString::fromUserString(k)));
		byCategory->add(std::make_shared<UILabel>("", labelLightStyle, LocalisedString::fromUserString(getNumberWithCommas(v))));

		auto keys = origStats.keysPerCategory.at(k);
		byCategory->add(std::make_shared<UILabel>("", labelLightStyle, LocalisedString::fromUserString(getNumberWithCommas(keys))));
		byCategory->add(std::make_shared<UILabel>("", labelLightStyle, LocalisedString::fromUserString(toString(static_cast<float>(v) / static_cast<float>(keys), 1))));
	}

	auto languagesContainer = getWidget("languages");
	languagesContainer->clear();

	for (const auto& lang: project.getProperties().getLanguages()) {
		if (lang != originalLanguage.language) {
			addTranslationData(*languagesContainer, lang, origStats.totalKeys);
		}
	}
}

void LocalisationEditor::addTranslationData(UIWidget& container, const I18NLanguage& language, int totalKeys)
{
	auto widget = factory.makeUI("halley/localisation_language_summary");
	widget->layout();

	widget->getWidgetAs<UIImage>("flag")->setSprite(getFlag(language));
	widget->getWidgetAs<UILabel>("languageName")->setText(LocalisedString::fromUserString(getLanguageName(language)));

	const auto iter = localised.find(language.getISOCode());
	const auto locData = iter == localised.end() ? LocalisationData{} : iter->second;
	const auto locStats = locData.getTranslationStats(originalLanguage);

	const int translatedPercent = std::max((locStats.translatedKeys * 100) / totalKeys, locStats.translatedKeys > 0 ? 1 : 0);

	const auto rect = Rect4i(widget->getWidget("bar_full")->getRect());
	const int totalW = rect.getWidth() - 2;
	const int totalH = rect.getHeight();
	const int greenW = std::max((locStats.translatedKeys * totalW) / totalKeys, locStats.translatedKeys > 0 ? 1 : 0);
	const int yellowW = std::max((locStats.outdatedKeys * totalW) / totalKeys, locStats.outdatedKeys > 0 ? 1 : 0);

	widget->getWidgetAs<UILabel>("completion")->setText(LocalisedString::fromUserString(toString(translatedPercent) + "%"));
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
	countryNames["NL"] = "Netherlands";
	countryNames["CZ"] = "Czechia";
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
	languageNames["ar"] = "Arabic";
	languageNames["nl"] = "Dutch";
	languageNames["cs"] = "Czech";

	languageNeedsQualifier.insert("en");
	languageNeedsQualifier.insert("pt");
	languageNeedsQualifier.insert("zh");
	languageNeedsQualifier.insert("fr");
	languageNeedsQualifier.insert("es");
}
