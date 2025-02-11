#include "localisation_editor.h"

#include "localisation_editor_root.h"
#include "localisation_language_editor.h"
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

LocalisationEditor::LocalisationEditor(LocalisationEditorRoot& root, ProjectWindow& projectWindow, UIFactory& factory)
	: UIWidget("localisation_editor", {}, UISizer())
	, root(root)
    , projectWindow(projectWindow)
	, project(projectWindow.getProject())
	, factory(factory)
	, api(projectWindow.getAPI())
	, aliveFlag(std::make_shared<bool>(true))
{
}

LocalisationEditor::~LocalisationEditor()
{
	*aliveFlag = false;
}

void LocalisationEditor::onMakeUI()
{
	if (isDevEnvironment()) {
		loadOriginalDataFromDisk();
	}

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

	const auto& credentials = projectWindow.getSetting(EditorSettingType::Project, "localisation_credentials");
	getWidgetAs<UITextInput>("username")->setText(credentials["username"].asString(""));
	getWidgetAs<UITextInput>("password")->setText(credentials["password"].asString(""));
}

void LocalisationEditor::update(Time t, bool moved)
{
	tryLoading();

	if (localStringsFuture.isReady()) {
		localStrings = localStringsFuture.get();
		localStringsFuture = {};
		gotLocalStrings = true;
		populateData();
	}

	if (remoteStringsFuture.isReady()) {
		remoteStrings = remoteStringsFuture.get();
		remoteStringsFuture = {};
		state = State::Synchronised;
		curMessage = "Waiting on Strings...";
		gotRemoteStrings = true;
	}

	if (state == State::Synchronised && gotRemoteStrings && (!isDevEnvironment() || gotLocalStrings)) {
		state = State::Ready;
		curMessage = {};
		populateData();
	}

	getWidget("signInPanel")->setActive(state == State::NotConnected);
	getWidget("messagePanel")->setActive(curMessage.has_value());
	getWidget("toolbar")->setActive(state == State::Ready);
	getWidget("developerPanel")->setActive(isDevEnvironment());
	getWidget("originalLanguagePanel")->setActive(state == State::Ready || gotLocalStrings);
	getWidget("translationPanel")->setActive(state == State::Ready || gotLocalStrings);

	if (curMessage) {
		getWidgetAs<UILabel>("connectionMessage")->setText(LocalisedString::fromUserString(*curMessage));
	}
}

void LocalisationEditor::onActiveChanged(bool active)
{
}

void LocalisationEditor::onAssetsLoaded()
{
}

void LocalisationEditor::tryLoading()
{
	if (!loaded) {
		client = std::make_unique<LocalisationClient>(*api.web, project.getProperties().getLocalisationServer(), project.getBinName());
		factory.loadUI(*this, "halley/localisation_editor");
		project.addAssetLoadedListener(this);
		loaded = true;
	}
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

	for (const auto& lang: getLanguages()) {
		if (lang != originalLanguage.getLanguage()) {
			bool canEdit = canEditLanguage(lang);
			if (canEdit || canViewLanguage(lang)) {
				if (auto* translation = getTranslationData(lang)) {
					addTranslationData(*languagesContainer, originalLanguage, *translation, getTranslationDataRemote(lang), origStats.totalKeys, origStats.totalWords, canEdit);
				}
			}
		}
	}
}

void LocalisationEditor::addTranslationData(UIWidget& container, const LocOriginalData& origData, const LocTranslationData& translationData, const LocTranslationData* translationDataRemote, int origTotalKeys, int totalWords, bool canEdit)
{
	const auto totalKeys = std::max(origTotalKeys, 1); // Avoid divisions by zero

	auto widget = factory.makeUI("halley/localisation_language_summary");
	widget->layout();

	widget->getWidgetAs<UIImage>("flag")->setSprite(root.getFlag(translationData.language));
	widget->getWidgetAs<UILabel>("languageName")->setText(root.getLanguageName(translationData.language));
	widget->getWidgetAs<UIButton>("edit")->setLabel(LocalisedString::fromHardcodedString(canEdit ? "Edit..." : "View..."));
	widget->getWidget("import")->setEnabled(canEdit);
	widget->getWidget("upload")->setEnabled(canEdit);

	const auto language = translationData.language;
	const auto locStats = translationData.getTranslationStats(origData);
	const auto locStatsRemote = translationDataRemote ? translationDataRemote->getTranslationStats(origData) : TranslationStats{};

	const auto translatedPercent = getPercent(locStats.translatedKeys, totalKeys);
	const auto translatedPercentRemote = getPercent(locStatsRemote.translatedKeys, totalKeys);

	const auto rect = Rect4i(widget->getWidget("bar_full")->getRect());
	const int totalW = rect.getWidth() - 2;
	const int totalH = rect.getHeight();
	const int blueW = std::max((locStatsRemote.translatedKeys * totalW) / totalKeys, locStatsRemote.translatedKeys > 0 ? 1 : 0);
	const int greenW = std::max((locStats.translatedKeys * totalW) / totalKeys, locStats.translatedKeys > 0 ? 1 : 0);
	const int yellowW = std::max((locStats.outdatedKeys * totalW) / totalKeys, locStats.outdatedKeys > 0 ? 1 : 0);

	auto percentString = toString(translatedPercent, 1) + "%";
	if (locStats.translatedKeys != locStatsRemote.translatedKeys) {
		percentString += " / " + toString(translatedPercentRemote, 1) + "%";
	}

	widget->getWidgetAs<UILabel>("completion")->setText(LocalisedString::fromUserString(percentString));
	widget->getWidgetAs<UIImage>("bar_blue")->setLocalClip(Rect4f(Rect4i(0, 0, blueW, totalH)));
	widget->getWidgetAs<UIImage>("bar_green")->setLocalClip(Rect4f(Rect4i(0, 0, greenW, totalH)));
	widget->getWidgetAs<UIImage>("bar_yellow")->setLocalClip(Rect4f(Rect4i(greenW, 0, yellowW, totalH)));

	auto cost = project.getProperties().getLanguageCost(language);
	widget->getWidget("costBox")->setActive(isDevEnvironment() && cost.has_value());
	if (cost) {
		widget->getWidgetAs<UILabel>("cost")->setText(LocalisedString::fromUserString(getCurrencyString(cost->first * totalWords, cost->second)));
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
	assert(remoteStrings || localStrings);

	return localStrings ? localStrings->originalLanguage : remoteStrings->originalLanguage;
}

LocOriginalData* LocalisationEditor::getOriginalDataRemote()
{
	if (localStrings && remoteStrings) {
		return &remoteStrings->originalLanguage;
	} else {
		return nullptr;
	}
}

LocTranslationData* LocalisationEditor::getTranslationData(const I18NLanguage& language)
{
	const auto code = language.getISOCode();
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

LocTranslationData* LocalisationEditor::getTranslationDataRemote(const I18NLanguage& language)
{
	const auto code = language.getISOCode();
	if (/*localStrings && */ remoteStrings) {
		if (const auto iter = remoteStrings->localised.find(code); iter != remoteStrings->localised.end()) {
			return &iter->second;
		} else {
			LocTranslationData data;
			data.language = language;
			remoteStrings->localised[code] = std::move(data);
			return &remoteStrings->localised.at(code);
		}
	}
	return nullptr;
}

void LocalisationEditor::openOriginalLanguage(bool canEdit)
{
	root.drillDown(std::make_shared<LocalisationLanguageEditor>(root, project, factory, getOriginalData(), nullptr, getOriginalDataRemote(), nullptr, canEdit));
}

void LocalisationEditor::openLanguage(const I18NLanguage& language, bool canEdit)
{
	root.drillDown(std::make_shared<LocalisationLanguageEditor>(root, project, factory, getOriginalData(), getTranslationData(language), getOriginalDataRemote(), getTranslationDataRemote(language), canEdit));
}

void LocalisationEditor::exportLanguage(const I18NLanguage& language)
{
	// TODO
}

void LocalisationEditor::importLanguage(const I18NLanguage& language)
{
	// TODO
}

void LocalisationEditor::uploadLanguage(const I18NLanguage& language)
{
	// TODO
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

void LocalisationEditor::signIn(const String& username, const String& password)
{
	curMessage = "Connecting...";
	state = State::Connecting;
	client->signIn(username, password).then(Executors::getMainUpdateThread(), [this, aliveFlag = aliveFlag] (LocalisationClient::LoginResult result)
	{
		if (*aliveFlag) {
			onConnected(result);
		}
	});
}

void LocalisationEditor::signOut()
{
	client->signOut();
	state = State::NotConnected;
}

void LocalisationEditor::onConnected(LocalisationClient::LoginResult result)
{
	if (result == LocalisationClient::LoginResult::Success) {
		state = State::Synchronising;
		int minVersion = 0;
		remoteStringsFuture = client->getStrings(project.getProperties().getOriginalLanguage(), minVersion);
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
	if (localStrings) {
		curMessage = "Uploading original strings...";
		client->postOriginalStrings(localStrings->originalLanguage).then([this, aliveFlag = aliveFlag] (bool result)
		{
			if (*aliveFlag) {
				if (result) {
					curMessage = {};
				} else {
					curMessage = "Error uploading original strings.";
				}
			}
		});
	}
}

void LocalisationEditor::downloadTranslations()
{
	if (remoteStrings) {
		const auto& orig = remoteStrings->originalLanguage;
		
		for (const auto& [langId, localisedData]: remoteStrings->localised) {
			const auto lang = I18NLanguage(langId);

			std::stringstream str;
			str << lang.getISOCode().cppStr() << ":\n";
			int nEntries = 0;

			for (const auto& chunk: orig.getChunks()) {
				bool firstInChunk = true;

				for (const auto& entry: chunk.entries) {
					if (const auto iter = localisedData.entries.find(entry.key); iter != localisedData.entries.end()) {
						if (firstInChunk) {
							str << "\n\t# " << chunk.name << "\n";
							firstInChunk = false;
						}

						str << "\t" << entry.key << ": \"" << iter->second.value.replaceAll("\"", "\\\"") << "\"\n";
						++nEntries;
					}
				}
			}

			const auto dirPath = project.getAssetsSrcPath() / "config" / "strings" / "localised";
			const auto path = dirPath / (lang.getISOCode() + ".yaml");
			if (nEntries > 0 || Path::exists(path)) {
				if (nEntries == 0) {
					str << "\t{}";
				}
				FileSystem::createDir(dirPath);
				Path::writeFile(path, str.str());
			}
		}
	}
}
