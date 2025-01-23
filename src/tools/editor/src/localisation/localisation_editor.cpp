#include "localisation_editor.h"

#include "localisation_editor_root.h"
#include "localisation_language_editor.h"
#include "halley/tools/project/project.h"
#include "halley/tools/project/project_properties.h"

using namespace Halley;

namespace {
	String getNumberWithCommas(int number)
	{
		if (number >= 1'000'000) {
			return toString(number / 1'000'000) + "," + toString((number % 1000000) / 1000, 10, 3) + "," + toString(number % 1000, 10, 3);
		} else if (number >= 1000) {
			return toString(number / 1000) + "," + toString(number % 1000, 10, 3);
		} else {
			return toString(number);
		}
	}

	float getPercent(int cur, int total)
	{
		// Note: round to closest per thousand, don't report 0 unless there are 0 keys, don't report 100% unless we have every key
		if (cur == 0) {
			return 0.0f;
		} else if (cur == total) {
			return 100.0f;
		}

		const auto result = std::round(static_cast<float>(cur) * 1000.0f / static_cast<float>(total)) / 10.0f;
		return clamp(result, 0.1f, 99.9f);
	}

	String getCurrencyString(float value, const String& currency)
	{
		if (currency == "GBP") {
			return u8"£" + getNumberWithCommas((int)value) + "." + toString(int((value - (int)value) * 100), 10, 2);
		}
		if (currency == "USD") {
			return u8"$" + getNumberWithCommas((int)value) + "." + toString(int((value - (int)value) * 100), 10, 2);
		}
		return toString(value, 2) + " " + currency;
	}
}

LocalisationInfoRetriever::LocalisationInfoRetriever(Project& project)
	: project(project)
{
}

String LocalisationInfoRetriever::getCategory(const String& assetId) const
{
	if (project.getGameInstance()) {
		return project.getGameInstance()->getLocalisationFileCategory(assetId);
	} else {
		return "unknown";
	}
}

LocalisationEditor::LocalisationEditor(LocalisationEditorRoot& root, Project& project, UIFactory& factory, const HalleyAPI& api)
	: UIWidget("localisation_editor", {}, UISizer())
	, root(root)
    , project(project)
	, factory(factory)
	, api(api)
{
}

void LocalisationEditor::update(Time t, bool moved)
{
	if (!loaded && project.isDLLLoaded()) {
		load();
	}

	if (localStringsFuture.isReady()) {
		localStrings = localStringsFuture.get();
		localStringsFuture = {};
		populateData();
	}

	if (remoteStringsFuture.isReady()) {
		remoteStrings = remoteStringsFuture.get();
		remoteStringsFuture = {};
		populateData();
	}
}

void LocalisationEditor::onMakeUI()
{
	requestPopulateDataFromResources();

	setHandle(UIEventType::ButtonClicked, "upload", [=] (const UIEvent& event)
	{
		uploadOriginalStrings();
	});

	setHandle(UIEventType::ButtonClicked, "editOriginal", [this] (const UIEvent& event)
	{
		openOriginalLanguage(true);
	});
}

void LocalisationEditor::onActiveChanged(bool active)
{
	if (active && project.isDLLLoaded()) {
		//requestPopulateData();
	}
}

void LocalisationEditor::onAssetsLoaded()
{
	if (isActiveInHierarchy() && project.isDLLLoaded()) {
		requestPopulateDataFromResources();
	}
}

void LocalisationEditor::load()
{
	loaded = true;
	factory.loadUI(*this, "halley/localisation_editor");
	project.addAssetLoadedListener(this);

	client = std::make_unique<LocalisationClient>(*api.web, "http://localhost:8080", "witchbrook");
	loadCurrentStrings();
}

void LocalisationEditor::loadData()
{
	loadOriginalDataFromDisk();
}

void LocalisationEditor::loadOriginalDataFromDisk()
{
	localStringsFuture = Concurrent::execute([info = LocalisationInfoRetriever(project)]() -> Result
	{
		Result result;
		auto& project = info.getProject();

		// Scan for original language
		const auto origLanguageCode = project.getProperties().getOriginalLanguage();
		result.originalLanguage = LocOriginalData::generateFromProject(origLanguageCode, project, info);

		// Scan for localisation from HDD
		for (const auto& lang: project.getProperties().getLanguages()) {
			if (lang != origLanguageCode) {
				result.localised[lang.getISOCode()] = LocTranslationData::generateFromProject(lang, project);
			}
		}

		return result;
	});
}

void LocalisationEditor::requestPopulateDataFromResources()
{
	if (loaded) {
		loadData();
	} else {
		if (project.isDLLLoaded()) {
			load(); // Calls loadFromResources();
		} else {
			return;
		}
	}
}

void LocalisationEditor::populateData()
{
	if (!localStrings && !remoteStrings) {
		return;
	}

	populateOriginalLanguageData();
	populateTranslationData();
}

void LocalisationEditor::populateOriginalLanguageData()
{
	auto& originalLanguage = getOriginalData();
	const auto origStats = originalLanguage.getStats();

	getWidgetAs<UIImage>("mainLanguageFlag")->setSprite(root.getFlag(originalLanguage.getLanguage()));
	getWidgetAs<UILabel>("mainLanguage")->setText(root.getLanguageName(originalLanguage.getLanguage()));
	getWidgetAs<UILabel>("wordCount")->setText(LocalisedString::fromUserString(getNumberWithCommas(origStats.totalWords)));
	getWidgetAs<UILabel>("keyCount")->setText(LocalisedString::fromUserString(getNumberWithCommas(origStats.totalKeys)));

	HashMap<String, float> costPerWord;
	for (auto& lang: project.getProperties().getLanguages()) {
		if (auto cost = project.getProperties().getLanguageCost(lang)) {
			costPerWord[cost->second] += cost->first;
		}
	}
	Vector<String> costStrs;
	for (const auto& [currency, cost]: costPerWord) {
		costStrs += getCurrencyString(cost * origStats.totalWords, currency);
	}
	getWidgetAs<UILabel>("totalCost")->setText(LocalisedString::fromUserString(String::concatList(costStrs, " + ")));

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

	bool canEditOriginal = canEditLanguage(originalLanguage.getLanguage());
	getWidgetAs<UIButton>("editOriginal")->setLabel(LocalisedString::fromHardcodedString(canEditOriginal ? "Edit Original..." : "View Original..."));
}

void LocalisationEditor::populateTranslationData()
{
	auto& originalLanguage = getOriginalData();
	const auto origStats = originalLanguage.getStats();

	auto languagesContainer = getWidget("languages");
	languagesContainer->clear();

	for (const auto& lang: project.getProperties().getLanguages()) {
		if (lang != originalLanguage.getLanguage()) {
			bool canEdit = canEditLanguage(lang);
			if (canEdit || canViewLanguage(lang)) {
				if (auto* translation = getTranslationData(lang)) {
					addTranslationData(*languagesContainer, originalLanguage, *translation, origStats.totalKeys, origStats.totalWords, canEdit);
				}
			}
		}
	}
}

void LocalisationEditor::addTranslationData(UIWidget& container, const LocOriginalData& origData, const LocTranslationData& translationData, int origTotalKeys, int totalWords, bool canEdit)
{
	const auto totalKeys = std::max(origTotalKeys, 1); // Avoid divisions by zero

	auto widget = factory.makeUI("halley/localisation_language_summary");
	widget->layout();

	widget->getWidgetAs<UIImage>("flag")->setSprite(root.getFlag(translationData.language));
	widget->getWidgetAs<UILabel>("languageName")->setText(root.getLanguageName(translationData.language));
	widget->getWidgetAs<UIButton>("edit")->setLabel(LocalisedString::fromHardcodedString(canEdit ? "Edit..." : "View..."));

	const auto locStats = translationData.getTranslationStats(origData);

	const auto translatedPercent = getPercent(locStats.translatedKeys, totalKeys);

	const auto rect = Rect4i(widget->getWidget("bar_full")->getRect());
	const int totalW = rect.getWidth() - 2;
	const int totalH = rect.getHeight();
	const int greenW = std::max((locStats.translatedKeys * totalW) / totalKeys, locStats.translatedKeys > 0 ? 1 : 0);
	const int yellowW = std::max((locStats.outdatedKeys * totalW) / totalKeys, locStats.outdatedKeys > 0 ? 1 : 0);

	widget->getWidgetAs<UILabel>("completion")->setText(LocalisedString::fromUserString(toString(translatedPercent, 1) + "%"));
	widget->getWidgetAs<UIImage>("bar_green")->setLocalClip(Rect4f(Rect4i(0, 0, greenW, totalH)));
	widget->getWidgetAs<UIImage>("bar_yellow")->setLocalClip(Rect4f(Rect4i(greenW, 0, yellowW, totalH)));

	auto cost = project.getProperties().getLanguageCost(translationData.language);
	widget->getWidget("costBox")->setActive(cost.has_value());
	if (cost) {
		widget->getWidgetAs<UILabel>("cost")->setText(LocalisedString::fromUserString(getCurrencyString(cost->first * totalWords, cost->second)));
	}

	widget->setHandle(UIEventType::ButtonClicked, "edit", [this, language = translationData.language, canEdit] (const UIEvent& event)
	{
		openLanguage(language, canEdit);
	});

	container.add(widget);
}

LocOriginalData& LocalisationEditor::getOriginalData()
{
	assert(remoteStrings || localStrings);

	return localStrings ? localStrings->originalLanguage : remoteStrings->originalLanguage;
}

const LocOriginalData& LocalisationEditor::getOriginalData() const
{
	assert(remoteStrings || localStrings);

	return localStrings ? localStrings->originalLanguage : remoteStrings->originalLanguage;
}

LocTranslationData* LocalisationEditor::getTranslationData(const I18NLanguage& language)
{
	const auto code = language.getLanguageCode();
	if (remoteStrings) {
		if (const auto iter = remoteStrings->localised.find(code); iter != remoteStrings->localised.end()) {
			return &iter->second;
		}
	}
	if (localStrings) {
		if (const auto iter = localStrings->localised.find(code); iter != localStrings->localised.end()) {
			return &iter->second;
		}
		LocTranslationData data;
		data.language = language;
		localStrings->localised[code] = std::move(data);
		return &localStrings->localised.at(code);
	}
	return nullptr;
}

void LocalisationEditor::openOriginalLanguage(bool canEdit)
{
	root.drillDown(std::make_shared<LocalisationLanguageEditor>(root, project, factory, getOriginalData(), nullptr, canEdit));
}

void LocalisationEditor::openLanguage(const I18NLanguage& language, bool canEdit)
{
	root.drillDown(std::make_shared<LocalisationLanguageEditor>(root, project, factory, getOriginalData(), getTranslationData(language), canEdit));
}

bool LocalisationEditor::canViewLanguage(const I18NLanguage& language) const
{
	// TODO
	return true;
}

bool LocalisationEditor::canEditLanguage(const I18NLanguage& language) const
{
	// TODO
	return true;
}

void LocalisationEditor::loadCurrentStrings()
{
	int minVersion = 0;
	remoteStringsFuture = client->getStrings(minVersion);
}

void LocalisationEditor::uploadOriginalStrings()
{
	if (localStrings) {
		client->postOriginalStrings(localStrings->originalLanguage).then([] (bool result)
		{
			Logger::logInfo("Done posting to server, result was: " + toString(result));
		});
	}
}
