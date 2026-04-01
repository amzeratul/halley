#include "localisation_editor.h"

#include "localisation_editor_root.h"
#include "localisation_export_window.h"
#include "localisation_language_editor.h"
#include "localisation_manage_users.h"
#include "localisation_upload_strings_window.h"
#include "halley/tools/file/filesystem.h"
#include "halley/tools/project/project.h"
#include "halley/tools/project/project_properties.h"
#include "src/ui/project_window.h"

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
			return String(U'\u00A3') + getNumberWithCommas((int)value) + "." + toString(int((value - (int)value) * 100), 10, 2);
		}
		if (currency == "USD") {
			return String(U'$') + getNumberWithCommas((int)value) + "." + toString(int((value - (int)value) * 100), 10, 2);
		}
		if (currency == "EUR") {
			return String(U'\u20AC') + getNumberWithCommas((int)value) + "." + toString(int((value - (int)value) * 100), 10, 2);
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

LocalisationEditor::LocalisationEditor(LocalisationEditorRoot& root, ProjectWindow& projectWindow, UIFactory& factory)
	: UIWidget("localisation_editor", {}, UISizer())
	, root(root)
    , projectWindow(projectWindow)
	, project(projectWindow.getProject())
	, factory(factory)
	, api(projectWindow.getAPI())
{
	setupCurrencyConversion();
	if (!project.getProperties().isDevEnvironment()) {
		storageContainer = api.system->getStorageContainer(SaveDataType::SaveLocal, "loc_data_" + project.getBinName());
	}
}

void LocalisationEditor::onMakeUI()
{
	loadLocalStrings();

	setHandle(UIEventType::ButtonClicked, "upload", [=] (const UIEvent& event)
	{
		uploadOriginalStrings();
	});

	setHandle(UIEventType::ButtonClicked, "download", [=] (const UIEvent& event)
	{
		downloadTranslations();
	});

	setHandle(UIEventType::ButtonClicked, "editOriginal", [this] (const UIEvent& event)
	{
		openOriginalLanguage(true);
	});

	setHandle(UIEventType::ButtonClicked, "signIn", [this] (const UIEvent& event)
	{
		auto username = getWidgetAs<UITextInput>("username")->getText();
		auto password = getWidgetAs<UITextInput>("password")->getText();

		ConfigNode credentials;
		credentials["username"] = username;
		credentials["password"] = password;
		projectWindow.setSetting(EditorSettingType::Project, "localisation_credentials", std::move(credentials));

		signIn(username, password);
	});

	setHandle(UIEventType::ButtonClicked, "signOut", [this] (const UIEvent& event)
	{
		signOut();
	});

	setHandle(UIEventType::ButtonClicked, "manageUsers", [this] (const UIEvent& event)
	{
		manageUsers();
	});

	setHandle(UIEventType::ButtonClicked, "manageProject", [this] (const UIEvent& event)
	{
		manageProject();
	});

	const auto& credentials = projectWindow.getSetting(EditorSettingType::Project, "localisation_credentials");
	getWidgetAs<UITextInput>("username")->setText(credentials["username"].asString(""));
	getWidgetAs<UITextInput>("password")->setText(credentials["password"].asString(""));
}

void LocalisationEditor::update(Time t, bool moved)
{
	tryLoading();

	getWidget("signInPanel")->setActive(state == State::NotConnected);
	getWidget("messagePanel")->setActive(curMessage.has_value());
	getWidget("toolbar")->setActive(state == State::Ready);
	getWidget("manageUsers")->setActive(state == State::Ready && client->isAdmin());
	getWidget("manageProject")->setActive(isDevEnvironment() && state == State::Ready && client->isAdmin());
	getWidget("developerPanel")->setActive(isDevEnvironment());
	getWidget("originalLanguagePanel")->setActive(state == State::Ready || (gotLocalStrings && localStrings->originalLanguage));
	getWidget("translationPanel")->setActive(state == State::Ready || (gotLocalStrings && localStrings->originalLanguage));

	if (curMessage) {
		getWidgetAs<UILabel>("connectionMessage")->setText(LocalisedString::fromUserString(*curMessage));
	}
}

void LocalisationEditor::onEditorRootUpdate(Time t)
{
	tryLoading();

	if (localStringsFuture.isReady()) {
		localStrings = localStringsFuture.get();
		root.getRemoteClientUpdater().setLocalStrings(&*localStrings);
		localStringsFuture = {};
		gotLocalStrings = true;
		if (gotRemoteStrings) {
			pendingRemoteStrings = false;
			onStringsReady(firstUpdate);
			firstUpdate = false;
		} else {
			populateData();
		}
	}

	if (remoteStringsFuture.isReady()) {
		if (auto result = remoteStringsFuture.get()) {
			onRemoteStringsReceived(std::move(*result));
			remoteStringsFuture = {};
			if (state == State::Synchronising) {
				state = State::Synchronised;
				curMessage = "Waiting on Strings...";
			}
		} else {
			if (state == State::Synchronising) {
				remoteStringsFuture = {};
				state = State::NotConnected;
				curMessage = "Unable to retrieve Strings.";
			}
		}
	}

	if (state == State::Synchronised && gotRemoteStrings && gotLocalStrings) {
		state = State::Ready;
		curMessage = {};
	}

	if (pendingRemoteStrings && state == State::Ready) {
		pendingRemoteStrings = false;
		onStringsReady(firstUpdate);
		firstUpdate = false;
	}

	updateCheckForNewStrings(t);
}

void LocalisationEditor::onActiveChanged(bool active)
{
}

void LocalisationEditor::onAssetsLoaded()
{
	assetsChanged = true;
}

void LocalisationEditor::onReturnedFromDrillDown()
{
	checkForNewStrings();
	onLocalStringsModified();
}

void LocalisationEditor::tryLoading()
{
	if (!loaded) {
		client = std::make_unique<LocalisationClient>(*api.web, project.getProperties().getLocalisationServer(), project.getBinName(), project.getProperties().getOriginalLanguage());
		factory.loadUI(*this, "halley/localisation/localisation_editor");
		project.addAssetLoadedListener(this);
		loaded = true;
	}

	if (assetsChanged) {
		assetsChanged = false;
		loadLocalStrings();
	}
}

void LocalisationEditor::loadLocalStrings()
{
	if (localStringsFuture.isValid()) {
		return;
	}

	if (isDevEnvironment()) {
		loadOriginalDataFromDisk();
	} else {
		loadLocalStringsFromStorage();
	}
}

void LocalisationEditor::loadOriginalDataFromDisk()
{
	localStringsFuture = Concurrent::execute([info = LocalisationInfoRetriever(project)]() -> LocStringSet
	{
		LocStringSet result;
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

void LocalisationEditor::loadLocalStringsFromStorage()
{
	if (!isDevEnvironment()) {
		const auto bytes = storageContainer->getData("localStrings");
		if (!bytes.empty()) {
			const auto& configFile = Deserializer::fromBytes<ConfigFile>(bytes, SerializerOptions(1));
			localStringsFuture = Future<LocStringSet>::makeImmediate(LocStringSet(configFile.getRoot()));
		} else {
			localStringsFuture = Future<LocStringSet>::makeImmediate(LocStringSet());
		}
	}
}

void LocalisationEditor::saveLocalStringsToStorage()
{
	if (!isDevEnvironment()) {
		storageContainer->setData("localStrings", Serializer::toBytes(ConfigFile(localStrings->toConfigNode()), SerializerOptions(1)));
	}
}

void LocalisationEditor::onLocalStringsModified()
{
	saveLocalStringsToStorage();
	populateData();
}

void LocalisationEditor::onStringsReady(bool forceUpdate)
{
	const bool updated = updateLocalFromRemote();
	if (!updated && forceUpdate) {
		// The above will call this already if it detects local changes
		populateData();
	}
}

bool LocalisationEditor::updateLocalFromRemote()
{
	if (!localStrings || !remoteStrings) {
		return false;
	}

	bool modified = false;

	if (localStrings->originalLanguage && remoteStrings->originalLanguage) {
		modified = localStrings->originalLanguage->updateLocalFromRemote(*remoteStrings->originalLanguage) || modified;
	}

	for (auto& remoteLoc: remoteStrings->localised) {
		auto& locData = localStrings->getLocalised(I18NLanguage(remoteLoc.first));
		if (remoteStrings->originalLanguage) {
			modified = locData.pruneKeys(*remoteStrings->originalLanguage) || modified;
		}
		modified = locData.updateLocalFromRemote(remoteLoc.second) || modified;
	}

	if (modified) {
		onLocalStringsModified();
	}
	return modified;
}

void LocalisationEditor::populateData()
{
	if ((localStrings && localStrings->originalLanguage) || remoteStrings) {
		populateOriginalLanguageData();
		populateTranslationData();
		root.onStringsUpdated();
	}
}

void LocalisationEditor::populateOriginalLanguageData()
{
	const auto& filterRules = project.getProperties().getLocFilterRules();
	auto& originalLanguage = getOriginalData();
	const auto origStats = originalLanguage.getStats(filterRules);

	getWidgetAs<UIImage>("mainLanguageFlag")->setSprite(root.getFlag(originalLanguage.getLanguage()));
	getWidgetAs<UILabel>("mainLanguage")->setText(root.getLanguageName(originalLanguage.getLanguage()));
	getWidgetAs<UILabel>("wordCount")->setText(LocalisedString::fromUserString(getNumberWithCommas(origStats.totalWords)));
	getWidgetAs<UILabel>("keyCount")->setText(LocalisedString::fromUserString(getNumberWithCommas(origStats.totalKeys)));

	const auto isDev = isDevEnvironment();
	getWidget("byCategoryPanel")->setActive(isDev);
	getWidget("totalCostPanel")->setActive(isDev);

	if (isDev) {
		// Total cost
		Vector<String> costStrs;
		for (const auto& [currency, cost]: getLocCosts("GBP")) {
			costStrs += getCurrencyString(cost * origStats.totalWords, currency);
		}
		getWidgetAs<UILabel>("totalCost")->setText(LocalisedString::fromUserString(String::concatList(costStrs, " + ")));

		// Category breakdown
		auto labelStyle = factory.getStyle("label");
		auto labelLightStyle = factory.getStyle("labelLight");
		auto labelLightSubStyle = factory.getStyle("labelLightDisabled");
		auto byCategory = getWidget("byCategory");
		byCategory->clear();
		byCategory->add(std::make_shared<UILabel>("", labelStyle, LocalisedString::fromHardcodedString("Category")));
		byCategory->add(std::make_shared<UILabel>("", labelStyle, LocalisedString::fromHardcodedString("Words")), 0, {}, UISizerFillFlags::Right);
		byCategory->add(std::make_shared<UILabel>("", labelStyle, LocalisedString::fromHardcodedString("Keys")), 0, {}, UISizerFillFlags::Right);
		byCategory->add(std::make_shared<UILabel>("", labelStyle, LocalisedString::fromHardcodedString("% Ready")), 0, {}, UISizerFillFlags::Right);

		const auto addCategory = [&](const CategoryInfo& info, int level)
		{
			auto style = level == 0 ? labelLightStyle : labelLightSubStyle;
			byCategory->add(std::make_shared<UILabel>("", style, LocalisedString::fromUserString(info.id)), 0, Vector4f(float(level) * 10.0f, 0, 0, 0));
			byCategory->add(std::make_shared<UILabel>("", style, LocalisedString::fromUserString(getNumberWithCommas(info.words))), 0, {}, UISizerFillFlags::Right);
			byCategory->add(std::make_shared<UILabel>("", style, LocalisedString::fromUserString(getNumberWithCommas(info.keys))), 0, {}, UISizerFillFlags::Right);
			byCategory->add(std::make_shared<UILabel>("", style, LocalisedString::fromUserString(toString(100.0f * static_cast<float>(info.readyWords) / static_cast<float>(info.words), 1) + "%")), 0, {}, UISizerFillFlags::Right);
		};

		for (const auto& info: generateCategoryInfo(origStats)) {
			addCategory(info, 0);
			for (const auto& info2: info.children) {
				addCategory(info2, 1);
			}
		}
	}

	bool canEditOriginal = canEditLanguage(originalLanguage.getLanguage());
	getWidgetAs<UIButton>("editOriginal")->setLabel(LocalisedString::fromHardcodedString(canEditOriginal ? "Edit Original..." : "View Original..."));
}

Vector<LocalisationEditor::CategoryInfo> LocalisationEditor::generateCategoryInfo(const LocalisationStats& stats) const
{
	Vector<CategoryInfo> result;

	auto getOrMakeCategory = [&](const String& id) -> CategoryInfo&
	{
		for (auto& c: result) {
			if (c.id == id) {
				return c;
			}
		}
		auto& r = result.emplace_back();
		r.id = id;
		return r;
	};

	for (const auto& [categoryId, nWords]: stats.wordsPerCategory) {
		auto splitCategory = categoryId.split('/');
		HalleyAssertDev(splitCategory.size() >= 1 && splitCategory.size() <= 2);

		CategoryInfo info;
		info.id = splitCategory.back();
		info.words = nWords;
		info.keys = stats.keysPerCategory.at(categoryId);
		info.readyWords = stats.readyPerCategory.at(categoryId);

		if (splitCategory.size() == 2) {
			auto& parent = getOrMakeCategory(splitCategory.front());
			parent += info;
			parent.children += info;
		} else {
			result += info;
		}
	}

	std::sort(result.begin(), result.end());
	for (auto& r: result) {
		std::sort(r.children.begin(), r.children.end());
	}

	return result;
}

void LocalisationEditor::populateTranslationData()
{
	const auto& filterRules = project.getProperties().getLocFilterRules();
	auto& originalLanguage = getOriginalData();
	const auto origStats = originalLanguage.getStats(filterRules);

	auto languagesContainer = getWidget("languages");
	languagesContainer->clear();

	for (const auto& lang: getLanguages()) {
		if (lang != originalLanguage.getLanguage()) {
			bool canEdit = canEditLanguage(lang);
			if (canEdit || canViewLanguage(lang)) {
				if (auto* translation = getTranslationData(lang)) {
					addTranslationData(*languagesContainer, originalLanguage, *translation, getTranslationDataRemote(lang), origStats, canEdit);
				}
			}
		}
	}
}

void LocalisationEditor::addTranslationData(UIWidget& container, const LocOriginalData& origData, const LocTranslationData& translationData, const LocTranslationData* translationDataRemote, const LocalisationStats& origStats, bool canEdit)
{
	const auto totalWords = std::max(origStats.totalWords, 1); // Avoid divisions by zero

	auto widget = factory.makeUI("halley/localisation/localisation_language_summary");
	widget->layout();

	widget->getWidgetAs<UIImage>("flag")->setSprite(root.getFlag(translationData.language));
	widget->getWidgetAs<UILabel>("languageName")->setText(root.getLanguageName(translationData.language));
	widget->getWidgetAs<UIButton>("edit")->setLabel(LocalisedString::fromHardcodedString(canEdit ? "Edit..." : "View..."));
	widget->getWidget("import")->setEnabled(canEdit);
	widget->getWidget("upload")->setEnabled(canEdit);

	const auto language = translationData.language;
	const auto locStats = translationData.getTranslationStats(origData, origStats);
	const auto locStatsRemote = translationDataRemote ? translationDataRemote->getTranslationStats(origData, origStats) : TranslationStats{};

	const auto translatedPercent = getPercent(locStats.translatedWords, totalWords);
	const auto translatedPercentRemote = getPercent(locStatsRemote.translatedWords, totalWords);

	const auto rect = Rect4i(widget->getWidget("bar_full")->getRect());
	const int totalW = rect.getWidth() - 2;
	const int totalH = rect.getHeight();
	const int greyW = std::max(((totalWords - origStats.readyWords) * totalW) / totalWords, origStats.readyWords == totalWords ? 0 : 1);
	const int blueW = std::max((locStatsRemote.translatedWords * totalW) / totalWords, locStatsRemote.translatedWords > 0 ? 1 : 0);
	const int greenW = std::max((locStats.translatedWords * totalW) / totalWords, locStats.translatedWords > 0 ? 1 : 0);
	const int yellowW = std::max((locStats.outdatedKeys * totalW) / totalWords, locStats.outdatedKeys > 0 ? 1 : 0);

	auto percentString = toString(translatedPercent, 1) + "%";
	if (translationDataRemote && locStats.translatedWords != locStatsRemote.translatedWords) {
		percentString += " / " + toString(translatedPercentRemote, 1) + "%";
	}

	widget->getWidgetAs<UILabel>("completion")->setText(LocalisedString::fromUserString(percentString));
	widget->getWidgetAs<UIImage>("bar_grey")->setLocalClip(Rect4f(Rect4i(totalW - greyW, 0, greyW, totalH)));
	widget->getWidgetAs<UIImage>("bar_blue")->setLocalClip(Rect4f(Rect4i(0, 0, blueW, totalH)));
	widget->getWidgetAs<UIImage>("bar_green")->setLocalClip(Rect4f(Rect4i(0, 0, greenW, totalH)));
	widget->getWidgetAs<UIImage>("bar_yellow")->setLocalClip(Rect4f(Rect4i(greenW, 0, yellowW, totalH)));

	widget->getWidget("upload")->setEnabled(translationDataRemote && translationData != *translationDataRemote);

	auto cost = project.getProperties().getLanguageCost(language);
	widget->getWidget("costBox")->setActive(isDevEnvironment() && cost.has_value());
	if (cost) {
		cost = convertCurrency(*cost, "GBP");
		widget->getWidgetAs<UILabel>("cost")->setText(LocalisedString::fromUserString(getCurrencyString(cost->first * origStats.totalWords, cost->second)));
	}

	widget->setHandle(UIEventType::ButtonClicked, "edit", [this, language, canEdit] (const UIEvent& event)
	{
		openLanguage(language, canEdit);
	});

	widget->setHandle(UIEventType::ButtonClicked, "export", [this, language] (const UIEvent& event)
	{
		exportLanguage(language);
	});

	widget->setHandle(UIEventType::ButtonClicked, "import", [this, language, canEdit] (const UIEvent& event)
	{
		if (canEdit) {
			importLanguage(language);
		}
	});

	widget->setHandle(UIEventType::ButtonClicked, "upload", [this, language, canEdit] (const UIEvent& event)
	{
		if (canEdit) {
			uploadLanguage(language);
		}
	});

	container.add(widget);
}

LocOriginalData& LocalisationEditor::getOriginalData()
{
	HalleyAssertDev(remoteStrings || localStrings);

	return localStrings && localStrings->originalLanguage ? *localStrings->originalLanguage : *remoteStrings->originalLanguage;
}

LocOriginalData* LocalisationEditor::getOriginalDataRemote()
{
	if (remoteStrings) {
		return &(*remoteStrings->originalLanguage);
	} else {
		return nullptr;
	}
}

LocTranslationData* LocalisationEditor::getTranslationData(const I18NLanguage& language)
{
	if (localStrings) {
		return &localStrings->getLocalised(language);
	}
	return nullptr;
}

LocTranslationData* LocalisationEditor::getTranslationDataRemote(const I18NLanguage& language)
{
	if (remoteStrings) {
		return &remoteStrings->getLocalised(language);
	}
	return nullptr;
}

void LocalisationEditor::openOriginalLanguage(bool canEdit)
{
	root.drillDown(std::make_shared<LocalisationLanguageEditor>(root, *client, project, factory, api, getOriginalData(), nullptr, getOriginalDataRemote(), nullptr, canEdit));
}

void LocalisationEditor::openLanguage(const I18NLanguage& language, bool canEdit)
{
	root.drillDown(std::make_shared<LocalisationLanguageEditor>(root, *client, project, factory, api, getOriginalData(), getTranslationData(language), getOriginalDataRemote(), getTranslationDataRemote(language), canEdit));
}

void LocalisationEditor::uploadLanguage(const I18NLanguage& language)
{
	if (localStrings && remoteStrings) {
		auto localisedDelta = localStrings->getLocalised(language).makeDeltaFrom(remoteStrings->getLocalised(language));

		if (remoteStrings->originalLanguage && isDevEnvironment()) {
			localisedDelta.updateOriginalVersions(*remoteStrings->originalLanguage);
		}

		client->putTranslatedStrings(localisedDelta);
	}
}

bool LocalisationEditor::isDevEnvironment() const
{
	return project.getProperties().isDevEnvironment();
}

bool LocalisationEditor::canViewLanguage(const I18NLanguage& language) const
{
	if (isDevEnvironment()) {
		return true;
	}
	return canEditLanguage(language);
}

bool LocalisationEditor::canEditLanguage(const I18NLanguage& language) const
{
	if (language == project.getProperties().getOriginalLanguage()) {
		return false;
		//return isDevEnvironment();
	}
	return client->getLanguages().contains("*") || client->getLanguages().contains(language.getISOCode());
}

Vector<I18NLanguage> LocalisationEditor::getLanguages() const
{
	auto projLangs = project.getProperties().getLanguages();
	if (localStrings) {
		for (const auto& loc: localStrings->localised) {
			auto lang = I18NLanguage(loc.first);
			if (!projLangs.contains(lang)) {
				projLangs.push_back(lang);
			}
		}
	}
	if (remoteStrings) {
		for (const auto& loc: remoteStrings->localised) {
			auto lang = I18NLanguage(loc.first);
			if (!projLangs.contains(lang)) {
				projLangs.push_back(lang);
			}
		}
	}
	for (auto& langId: client->getLanguages()) {
		if (langId != "*") {
			auto lang = I18NLanguage(langId);
			if (!projLangs.contains(lang)) {
				projLangs.push_back(lang);
			}
		}
	}
	return projLangs;
}

int LocalisationEditor::getHighestVersion(std::optional<String> chunk) const
{
	int highest = 0;

	const auto iter = highestVersions.find("*");
	if (iter != highestVersions.end()) {
		highest = iter->second;
	}

	if (chunk) {
		const auto iter2 = highestVersions.find(*chunk);
		if (iter2 != highestVersions.end()) {
			highest = std::max(highest, iter2->second);
		}
	}

	return highest;
}

void LocalisationEditor::updateCheckForNewStrings(Time t)
{
	const Time timeBetweenChecks = 3;//remoteStringsChunk ? 3 : 5;

	timeSinceLastStringCheck += t;
	if (timeSinceLastStringCheck >= timeBetweenChecks) {
		checkForNewStrings();
	}
}

void LocalisationEditor::checkForNewStrings()
{
	if (!remoteStrings || remoteStringsFuture.isValid()) {
		return;
	}

	timeSinceLastStringCheck = 0;

	const auto curVersion = getHighestVersion(remoteStringsChunk);

	client->getStringsVersion().then(aliveFlag, Executors::getMainUpdateThread(), [this, curVersion] (int latestVersion) {
		if (latestVersion > curVersion) {
			remoteStringsFuture = client->getStrings(remoteStringsChunk, curVersion + 1);
		}
	});
}

void LocalisationEditor::onRemoteStringsReceived(LocStringSet result)
{
	auto& highest = highestVersions[remoteStringsChunk.value_or("*")];
	highest = std::max(highest, result.highestVersion);

	if (gotRemoteStrings) {
		const bool changed = remoteStrings->updateWith(result);
		pendingRemoteStrings = changed;
		if (changed) {
			Logger::logInfo("Got updated remote Strings!");
		}
	} else {
		remoteStrings = std::move(result);
		root.getRemoteClientUpdater().setRemoteStrings(&*remoteStrings);
		gotRemoteStrings = true;
		pendingRemoteStrings = true;
	}
}

HashMap<String, float> LocalisationEditor::getLocCosts(std::optional<String> convertToCurrency) const
{
	HashMap<String, float> costPerCurrency;
	for (auto& lang: project.getProperties().getLanguages()) {
		if (auto cost = project.getProperties().getLanguageCost(lang)) {
			if (convertToCurrency && cost->second != convertToCurrency) {
				cost = convertCurrency(*cost, *convertToCurrency);
			}

			costPerCurrency[cost->second] += cost->first;
		}
	}
	return costPerCurrency;
}

std::pair<float, String> LocalisationEditor::convertCurrency(std::pair<float, String> cost, const String& dstCurrency) const
{
	if (auto result = convertCurrency(cost.first, cost.second, dstCurrency)) {
		return { *result, dstCurrency };
	} else {
		return cost;
	}
}

std::optional<float> LocalisationEditor::convertCurrency(float cost, const String& srcCurrency, const String& dstCurrency) const
{
	const auto srcValue = currencyDollarValues.contains(srcCurrency) ? std::optional(currencyDollarValues.at(srcCurrency)) : std::nullopt;
	const auto dstValue = currencyDollarValues.contains(dstCurrency) ? std::optional(currencyDollarValues.at(dstCurrency)) : std::nullopt;

	if (srcValue && dstValue) {
		return cost * (*srcValue / *dstValue);
	} else {
		return std::nullopt;
	}
}

void LocalisationEditor::setupCurrencyConversion()
{
	// TODO: don't hardcode this? :)
	// Could get it from https://api.fxratesapi.com/latest, but then gotta deal with it being async

	// Current as of 2025-05-21
	currencyDollarValues["USD"] = 1;
	currencyDollarValues["GBP"] = 1.34f;
	currencyDollarValues["EUR"] = 1.13f;
}

void LocalisationEditor::signIn(const String& username, const String& password)
{
	curMessage = "Connecting...";
	state = State::Connecting;
	client->signIn(username, password).then(aliveFlag, Executors::getMainUpdateThread(), [this] (LocalisationClient::LoginResult result)
	{
		onConnected(result);
	});
}

void LocalisationEditor::signOut()
{
	client->signOut();
	state = State::NotConnected;
	remoteStrings = {};
	remoteStringsChunk = {};
	remoteStringsFuture = {};
	highestVersions = {};
	gotRemoteStrings = false;
	firstUpdate = true;
	root.getRemoteClientUpdater().setRemoteStrings(nullptr);
}

void LocalisationEditor::onConnected(LocalisationClient::LoginResult result)
{
	if (result == LocalisationClient::LoginResult::Success) {
		state = State::Synchronising;
		remoteStringsFuture = client->getStrings();
		curMessage = "Synchronising...";
	} else if (result == LocalisationClient::LoginResult::ServerNotFound) {
		state = State::NotConnected;
		curMessage = "Could not connect to localisation server.";
	} else if (result == LocalisationClient::LoginResult::InvalidLogin) {
		state = State::NotConnected;
		curMessage = "Invalid username/password.";
	}
}

void LocalisationEditor::uploadOriginalStrings()
{
	if (localStrings && remoteStrings) {
		auto uploadData = LocStringUploadData(*localStrings->originalLanguage, *remoteStrings->originalLanguage);

		// Find all languages that have localised each string
		HashMap<String, Vector<String>> localisedIn;
		for (const auto& chunk: uploadData.getChunks()) {
			for (const auto& entry: chunk.entries) {
				for (const auto& [lang, data]: remoteStrings->localised) {
					if (data.entries.contains(entry.key)) {
						localisedIn[entry.key] += lang;
					}
				}
			}
		}

		getRoot()->addChild(std::make_shared<LocUploadStringsWindow>(factory, *client, std::move(uploadData), std::move(localisedIn)));
	}
}

void LocalisationEditor::downloadTranslations()
{
	if (!remoteStrings) {
		return;
	}

	const auto& orig = remoteStrings->originalLanguage;
	
	for (const auto& [langId, localisedData]: remoteStrings->localised) {
		const auto lang = I18NLanguage(langId);

		std::stringstream str;
		str << lang.getISOCode().cppStr() << ":\n";
		int nEntries = 0;

		for (const auto& chunk: orig->getChunks()) {
			bool firstInChunk = true;

			for (const auto& entry: chunk.entries) {
				if (const auto iter = localisedData.entries.find(entry.getKey()); iter != localisedData.entries.end()) {
					if (!iter->second.getValue().isEmpty()) {
						if (firstInChunk) {
							str << "\n  # " << chunk.name << "\n";
							firstInChunk = false;
						}

						const auto value = iter->second.getValue()
							.replaceAll("\"", "\\\"")
							.replaceAll("\r\n", "\\n")
							.replaceAll("\n", "\\n");
						str << "  " << entry.getKey() << ": \"" << value << "\"\n";
						++nEntries;
					}
				}
			}
		}

		const auto dirPath = project.getAssetsSrcPath() / "config" / "strings" / "localised";
		const auto path = dirPath / (lang.getISOCode() + ".yaml");
		if (nEntries > 0 || Path::exists(path)) {
			if (nEntries == 0) {
				str << "  {}";
			}
			FileSystem::createDir(dirPath);
			Path::writeFile(path, str.str());
		}
	}
}

void LocalisationEditor::exportLanguage(const I18NLanguage& language)
{
	if (!localStrings || !remoteStrings) {
		Logger::logError("Unable to export: strings not ready.");
		return;
	}

	Vector<String> chunks;
	chunks.reserve(getOriginalData().getChunks().size());
	for (const auto& chunk: getOriginalData().getChunks()) {
		chunks += chunk.name;
	}

	getRoot()->addChild(std::make_shared<LocalisationExportWindow>(factory, std::move(chunks), [=] (bool ok, LocalisationExportOptions options) {
		if (ok) {
			exportLanguage(language, options);
		}
	}));
}

void LocalisationEditor::exportLanguage(const I18NLanguage& language, const LocalisationExportOptions& options)
{
	auto basePath = project.getRootPath();

	FileChooserParameters fileChooserParams;
	fileChooserParams.defaultPath = basePath;
	fileChooserParams.fileName = project.getBinName() + "_" + language.getISOCode() + ".csv";
	fileChooserParams.fileTypes.emplace_back(FileChooserParameters::FileType{ "Comma-Separated Values", {"csv"}, true });
	//fileChooserParams.fileTypes.emplace_back(FileChooserParameters::FileType{ "YAML", {"yaml"}, false });
	fileChooserParams.save = true;

	OS::get().openFileChooser(fileChooserParams).then(Executors::getMainUpdateThread(), [this, language, options](std::optional<Path> path) {
		if (path) {
			doExportLanguage(language, options, *path);
		}
	});
}

void LocalisationEditor::doExportLanguage(const I18NLanguage& language, const LocalisationExportOptions& options, const Path& path)
{
	const auto& loc = *getTranslationData(language);
	const auto& orig = getOriginalData();

	CSVFile csv;
	csv.setColumns({{ "key", "priority", "ready", "original", "translation", "comment", "context" , "chunk", "version" }});

	const auto keyIdx = csv.getColumnIndex("key");
	const auto versionIdx = csv.getColumnIndex("version");
	const auto commentIdx = csv.getColumnIndex("comment");
	const auto contextIdx = csv.getColumnIndex("context");
	const auto priorityIdx = csv.getColumnIndex("priority");
	const auto readyIdx = csv.getColumnIndex("ready");
	const auto chunkIdx = csv.getColumnIndex("chunk");
	const auto originalIdx = csv.getColumnIndex("original");
	const auto translationIdx = csv.getColumnIndex("translation");

	const auto& filterRules = project.getProperties().getLocFilterRules();

	for (const auto& chunk: orig.getChunks()) {
		if (options.allChunks || options.chunksToInclude.contains(chunk.name)) {
			for (const auto& origEntry: chunk.entries) {
				const auto* locEntry = loc.tryGetEntry(origEntry.getKey());

				if (!options.filters.shouldShow(origEntry, locEntry, filterRules, language)) {
					continue;
				}

				auto rowIdx = csv.addRow();
				csv.setCell(rowIdx, keyIdx, origEntry.getKey());
				csv.setCell(rowIdx, versionIdx, toString(origEntry.getVersion()));
				csv.setCell(rowIdx, commentIdx, origEntry.getComment());
				csv.setCell(rowIdx, contextIdx, origEntry.getContext());
				csv.setCell(rowIdx, readyIdx, toString(origEntry.getReadyState(filterRules, language)));
				csv.setCell(rowIdx, priorityIdx, toString(origEntry.getPriority()));
				csv.setCell(rowIdx, originalIdx, origEntry.getValue());
				csv.setCell(rowIdx, chunkIdx, chunk.name);

				if (locEntry) {
					csv.setCell(rowIdx, translationIdx, locEntry->getValue());
				}
			}
		}
	}

	Path::writeFile(path, csv.save());
}

void LocalisationEditor::importLanguage(const I18NLanguage& language)
{
	if (!localStrings || !remoteStrings) {
		Logger::logError("Unable to import: strings not ready.");
		return;
	}

	auto basePath = project.getRootPath();

	FileChooserParameters fileChooserParams;
	fileChooserParams.defaultPath = basePath;
	fileChooserParams.fileName = "";
	fileChooserParams.fileTypes.emplace_back(FileChooserParameters::FileType{ "Comma-Separated Values", {"csv"}, true });
	fileChooserParams.fileTypes.emplace_back(FileChooserParameters::FileType{ "YAML", {"yaml"}, false });
	fileChooserParams.save = false;

	OS::get().openFileChooser(fileChooserParams).then(Executors::getMainUpdateThread(), [this, language](std::optional<Path> path) {
		if (path) {
			auto data = Path::readFile(*path);
			if (!data.empty()) {
				doImportLanguage(language, path->getExtension(), std::move(data));
			}
		}
	});
}

void LocalisationEditor::doImportLanguage(const I18NLanguage& language, const String& extension, Bytes data)
{
	if (extension == ".csv") {
		importLanguageFromCSV(language, data);
	} else if (extension == ".yaml" || extension == ".yml") {
		importLanguageFromYAML(language, data);
	} else {
		Logger::logError("Unknown extension for localisation import: \"" + extension + "\"");
	}
	onLocalStringsModified();
	uploadLanguage(language);
}

void LocalisationEditor::importLanguageFromYAML(const I18NLanguage& language, const Bytes& data)
{
	HalleyAssertDev(localStrings.has_value());
	HalleyAssertDev(remoteStrings.has_value());

	const auto configFile = YAMLConvert::parseConfig(data, {});
	const auto langId = language.getISOCode();
	if (!configFile.getRoot().hasKey(langId)) {
		Logger::logError("Failed to import YAML: not a localisation file for " + langId);
		return;
	}

	const auto& orig = *remoteStrings->originalLanguage;
	auto& translation = localStrings->localised[langId];
	translation.language = language;

	const auto& locRoot = configFile.getRoot()[langId];
	int n = 0;
	for (const auto& [k, v]: locRoot.asMap()) {
		if (translation.setValue(k, orig.getVersion(k), v.asString(""))) {
			++n;
		}
	}

	Logger::logInfo("Imported " + toString(n) + " keys to " + langId);
}

void LocalisationEditor::importLanguageFromCSV(const I18NLanguage& language, const Bytes& data)
{
	CSVFile csv;
	try {
		csv.load(data.const_byte_span());
	} catch (const std::exception& e) {
		Logger::logException(e);
		return;
	}

	const auto langId = language.getISOCode();
	auto& translation = localStrings->localised[langId];
	translation.language = language;

	const auto keyIdx = csv.getColumnIndex("key");
	const auto versionIdx = csv.getColumnIndex("version");
	const auto translationIdx = csv.getColumnIndex("translation");

	int n = 0;
	const auto nRows = csv.getNumRows();
	for (size_t i = 0; i < nRows; ++i) {
		const auto& key = csv.getCell(i, keyIdx);
		const auto* versionCell = csv.tryGetCell(i, versionIdx);
		const auto& translatedValue = csv.getCell(i, translationIdx);

		int version = -1;
		if (versionCell && versionCell->isInteger()) {
			version = versionCell->toInteger();
		} else {
			version = localStrings->originalLanguage->tryGetEntry(key)->getVersion();
		}

		if (!translatedValue.isEmpty()) {
			if (translation.setValue(key, version, translatedValue)) {
				++n;
			}
		}
	}

	Logger::logInfo("Imported " + toString(n) + " keys (out of " + toString(nRows) + " in file) to " + langId);
}

void LocalisationEditor::manageUsers()
{
	getRoot()->addChild(std::make_shared<LocalisationManageUsers>(factory, *client, project, root));
}

void LocalisationEditor::manageProject()
{
	const auto title = "Set project version";
	const auto msg = "Enter the Halley editor version to set the project to use.\nPlease make sure that the editor bin has been uploaded first!";

	getRoot()->addChild(std::make_shared<UIInputPopup>(factory, title, msg, getHalleyVersion().toString(), [=](std::optional<String> result) {
		if (result) {
			HalleyVersion version;
			version.parse(*result);
			if (version.isValid()) {
				uploadProjectProperties(version);
			}
		}
	}));
}

void LocalisationEditor::uploadProjectProperties(HalleyVersion version)
{
	HashMap<String, Bytes> files;

	auto properties = YAMLConvert::parseConfig(Path::readFile(project.getRootPath() / "halley_project" / "properties.yaml"));
	auto& nodeRoot = properties.getRoot();
	nodeRoot.removeKey("languageCosts");
	nodeRoot["devEnvironment"] = false;
	nodeRoot["halleyVersion"] = version.toString();

	files["halley_project/icon48.png"] = Path::readFile(project.getRootPath() / "halley_project" / "icon48.png");
	files["halley_project/properties.yaml"] = YAMLConvert::generateYAML(properties, {}).toBytes();

	client->putExternalProjectProperties(files).then(aliveFlag, Executors::getMainUpdateThread(), [=] (bool ok) {
		if (ok) {
			Logger::logInfo("Updated project info, using v" + version.toString());
		} else {
			Logger::logError("Unable to update project info");
		}
	});
}

bool LocalisationEditor::CategoryInfo::operator<(const CategoryInfo& other) const
{
	return words > other.words;
}

LocalisationEditor::CategoryInfo& LocalisationEditor::CategoryInfo::operator+=(const CategoryInfo& other)
{
	words += other.words;
	keys += other.keys;
	readyWords += other.readyWords;
	return *this;
}
