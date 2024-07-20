#include "localisation_editor_root.h"

#include "localisation_editor.h"

using namespace Halley;

LocalisationEditorRoot::LocalisationEditorRoot(Project& project, UIFactory& factory)
	: UIWidget("localisation_editor_root", {}, UISizer(UISizerType::Vertical))
	, project(project)
	, factory(factory)
{
	setupCountryNames();
	setupLanguageNames();

	editor = std::make_shared<LocalisationEditor>(*this, project, factory);

	add(editor, 1);
}

void LocalisationEditorRoot::drillDown(std::shared_ptr<UIWidget> widget)
{
	assert(!curWidget);

	curWidget = widget;
	add(curWidget, 1);
	editor->setActive(false);
}

void LocalisationEditorRoot::returnToRoot()
{
	assert(!!curWidget);

	curWidget->destroy();
	curWidget = {};
	editor->setActive(true);
}

void LocalisationEditorRoot::setupCountryNames()
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

void LocalisationEditorRoot::setupLanguageNames()
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

LocalisedString LocalisationEditorRoot::getLanguageName(const I18NLanguage& ln) const
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

	return LocalisedString::fromHardcodedString(country.isEmpty() ? language : language + " (" + country + ")");
}

Sprite LocalisationEditorRoot::getFlag(const I18NLanguage& language) const
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
