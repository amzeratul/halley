#pragma once

#include "localisation_client.h"
#include "localisation_data.h"
#include "halley/tools/project/project.h"
#include "halley/ui/ui_widget.h"

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
        ~LocalisationEditor() override;

        void update(Time t, bool moved) override;
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

        std::shared_ptr<bool> aliveFlag;
        std::unique_ptr<LocalisationClient> client;
        std::shared_ptr<ISaveData> storageContainer;

        enum class State {
	        NotConnected,
            Connecting,
            Synchronising,
            Synchronised,
            Ready
        };

        struct ExportOptions {
	        bool emitUntranslated = true;
	        bool emitOutOfDate = true;
	        bool emitTranslated = true;
            bool allChunks = true;
            HashSet<String> chunksToInclude;
        };

        enum class KeyState {
	        Untranslated,
            Translated,
            OutOfDate
        };

        bool loaded = false;
        bool gotLocalStrings = false;
        bool gotRemoteStrings = false;
        State state = State::NotConnected;
        std::optional<String> curMessage;

        std::optional<LocStringSet> localStrings;
        std::optional<LocStringSet> remoteStrings;
        Future<LocStringSet> localStringsFuture;
        Future<std::optional<LocStringSet>> remoteStringsFuture;

        void tryLoading();

        void loadLocalStrings();
        void loadOriginalDataFromDisk();
        void loadLocalStringsFromStorage();
        void saveLocalStringsToStorage();

        void onLocalStringsModified();
        void onRemoteStringsReceived();
        bool updateLocalFromRemote();

        void populateData();
        void populateOriginalLanguageData();
        void populateTranslationData();
        void addTranslationData(UIWidget& container, const LocOriginalData& origData, const LocTranslationData& translationData, const LocTranslationData* translationDataRemote, int totalKeys, int totalWords, bool canEdit);

        LocOriginalData& getOriginalData();
        LocOriginalData* getOriginalDataRemote();
        LocTranslationData* getTranslationData(const I18NLanguage& language);
        LocTranslationData* getTranslationDataRemote(const I18NLanguage& language);

        void openOriginalLanguage(bool canEdit);
        void openLanguage(const I18NLanguage& language, bool canEdit);

		void exportLanguage(const I18NLanguage& language);
        void exportLanguage(const I18NLanguage& language, const ExportOptions& options);
        void doExportLanguage(const I18NLanguage& language, const ExportOptions& options, const Path& path);

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

		void uploadOriginalStrings();
        void downloadTranslations();

        Vector<I18NLanguage> getLanguages() const;
    };
}
