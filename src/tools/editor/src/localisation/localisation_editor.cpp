#include "localisation_editor.h"

#include "localisation_editor_root.h"
#include "localisation_language_editor.h"
#include "halley/tools/project/project.h"
#include "halley/tools/project/project_properties.h"

using namespace Halley;

LocalisationEditor::LocalisationEditor(LocalisationEditorRoot& root, Project& project, UIFactory& factory)
	: UIWidget("localisation_editor", {}, UISizer())
	, root(root)
    , project(project)
	, factory(factory)
{
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
		//populateData();
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
	if (loaded) {
		loadFromResources();
	} else {
		if (project.isDLLLoaded()) {
			load(); // Calls loadFromResources();
		} else {
			return;
		}
	}

	const auto origStats = originalLanguage.getStats();
	getWidgetAs<UIImage>("mainLanguageFlag")->setSprite(root.getFlag(originalLanguage.language));
	getWidgetAs<UILabel>("mainLanguage")->setText(root.getLanguageName(originalLanguage.language));
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

	bool canEditOriginal = true; // TODO
	getWidgetAs<UIButton>("editOriginal")->setLabel(LocalisedString::fromHardcodedString(canEditOriginal ? "Edit Original..." : "View Original..."));
	setHandle(UIEventType::ButtonClicked, "editOriginal", [this] (const UIEvent& event)
	{
		openLanguage(originalLanguage, true);
	});

	auto languagesContainer = getWidget("languages");
	languagesContainer->clear();

	for (const auto& lang: project.getProperties().getLanguages()) {
		if (lang != originalLanguage.language) {
			bool canEdit = false; // TODO
			addTranslationData(*languagesContainer, lang, origStats.totalKeys, canEdit);
		}
	}
}

void LocalisationEditor::addTranslationData(UIWidget& container, const I18NLanguage& language, int totalKeys, bool canEdit)
{
	auto widget = factory.makeUI("halley/localisation_language_summary");
	widget->layout();

	widget->getWidgetAs<UIImage>("flag")->setSprite(root.getFlag(language));
	widget->getWidgetAs<UILabel>("languageName")->setText(root.getLanguageName(language));
	widget->getWidgetAs<UIButton>("edit")->setLabel(LocalisedString::fromHardcodedString(canEdit ? "Edit..." : "View..."));

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

	widget->setHandle(UIEventType::ButtonClicked, "edit", [this, language = language, canEdit] (const UIEvent& event)
	{
		openLanguage(localised.at(language.getISOCode()), canEdit);
	});

	container.add(widget);
}

void LocalisationEditor::openLanguage(LocalisationData& localisationData, bool canEdit)
{
	root.drillDown(std::make_shared<LocalisationLanguageEditor>(root, project, factory, originalLanguage, &localisationData == &originalLanguage ? nullptr : &localisationData, canEdit));
}
