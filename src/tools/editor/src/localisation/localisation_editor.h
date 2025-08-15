#pragma once

#include "localisation_client.h"
#include "localisation_data.h"
#include "localisation_filters.h"
#include "halley/tools/project/project.h"
#include "halley/ui/ui_widget.h"
#include "halley/text/halleystring.h"

namespace Halley {
	class ProjectWindow;
	class LocalisationEditorRoot;

    class LocalisationInfoRetriever : public ILocalisationInfoRetriever {
    public:
        LocalisationInfoRetriever(Project& project);

        String getCategory(const String& assetId) const override;
        Project& getProject() const { return project; }

    private:
        Project& project;
    };

	class LocalisationEditor : public UIWidget, public Project::IAssetLoadListener {
    public:
        LocalisationEditor(LocalisationEditorRoot& root, ProjectWindow& projectWindow, UIFactory& factory);

        void update(Time t, bool moved) override;
        void onEditorRootUpdate(Time t);
        void onMakeUI() override;
        void onActiveChanged(bool active) override;

        void onAssetsLoaded() override;
        void onReturnedFromDrillDown();

    private:
        LocalisationEditorRoot& root;
        ProjectWindow& projectWindow;
        Project& project;
        UIFactory& factory;
        const HalleyAPI& api;

        AliveFlag aliveFlag;
        std::unique_ptr<LocalisationClient> client;
        std::shared_ptr<ISaveData> storageContainer;

        Time timeSinceLastStringCheck = 0;

        enum class State {
	        NotConnected,
            Connecting,
            Synchronising,
            Synchronised,
            Ready
        };

        enum class KeyState {
	        Untranslated,
            Translated,
            OutOfDate
        };

        struct CategoryInfo {
            String id;
            int words;
            int keys;
            int readyWords;

            Vector<CategoryInfo> children;

            bool operator<(const CategoryInfo& other) const;
            CategoryInfo& operator+=(const CategoryInfo& other);
        };

        bool loaded = false;
        bool gotLocalStrings = false;
        bool gotRemoteStrings = false;
        bool pendingRemoteStrings = false;
        bool firstUpdate = true;
        State state = State::NotConnected;
        std::optional<String> curMessage;

        std::optional<LocStringSet> localStrings;
        std::optional<LocStringSet> remoteStrings;
        Future<LocStringSet> localStringsFuture;

		Future<std::optional<LocStringSet>> remoteStringsFuture;
        std::optional<String> remoteStringsChunk;
        HashMap<String, int> highestVersions;

        HashMap<String, float> currencyDollarValues;

        void tryLoading();

        void loadLocalStrings();
        void loadOriginalDataFromDisk();
        void loadLocalStringsFromStorage();
        void saveLocalStringsToStorage();

        void onLocalStringsModified();
        void onStringsReady(bool forceUpdate);
        bool updateLocalFromRemote();

        void populateData();
        void populateOriginalLanguageData();
        void populateTranslationData();
        void addTranslationData(UIWidget& container, const LocOriginalData& origData, const LocTranslationData& translationData, const LocTranslationData* translationDataRemote, const LocalisationStats& origStats, bool canEdit);

        LocOriginalData& getOriginalData();
        LocOriginalData* getOriginalDataRemote();
        LocTranslationData* getTranslationData(const I18NLanguage& language);
        LocTranslationData* getTranslationDataRemote(const I18NLanguage& language);
        Vector<CategoryInfo> generateCategoryInfo(const LocalisationStats& stats) const;

        void openOriginalLanguage(bool canEdit);
        void openLanguage(const I18NLanguage& language, bool canEdit);

		void exportLanguage(const I18NLanguage& language);
        void exportLanguage(const I18NLanguage& language, const LocalisationExportOptions& options);
        void doExportLanguage(const I18NLanguage& language, const LocalisationExportOptions& options, const Path& path);

		void importLanguage(const I18NLanguage& language);
        void doImportLanguage(const I18NLanguage& language, const String& extension, Bytes data);
        void importLanguageFromYAML(const I18NLanguage& language, const Bytes& data);
        void importLanguageFromCSV(const I18NLanguage& language, const Bytes& data);

		void uploadLanguage(const I18NLanguage& language);

        bool isDevEnvironment() const;
        bool canViewLanguage(const I18NLanguage& language) const;
        bool canEditLanguage(const I18NLanguage& language) const;

        void signIn(const String& username, const String& password);
        void signOut();
        void onConnected(LocalisationClient::LoginResult result);

        void manageUsers();
        void manageProject();
        void uploadProjectProperties(HalleyVersion version);

		void uploadOriginalStrings();
        void downloadTranslations();

        Vector<I18NLanguage> getLanguages() const;

        int getHighestVersion(std::optional<String> chunk) const;
        void updateCheckForNewStrings(Time t);
        void checkForNewStrings();
        void onRemoteStringsReceived(LocStringSet result);

        HashMap<String, float> getLocCosts(std::optional<String> convertToCurrency) const;
        std::pair<float, String> convertCurrency(std::pair<float, String> cost, const String& dstCurrency) const;
        std::optional<float> convertCurrency(float cost, const String& srcCurrency, const String& dstCurrency) const;
        void setupCurrencyConversion();
    };
}
