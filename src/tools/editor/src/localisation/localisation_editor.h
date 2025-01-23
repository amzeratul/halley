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

    private:
        LocalisationEditorRoot& root;
        ProjectWindow& projectWindow;
        Project& project;
        UIFactory& factory;
        const HalleyAPI& api;

        std::shared_ptr<bool> aliveFlag;
        std::unique_ptr<LocalisationClient> client;

        enum class State {
	        NotConnected,
            Connecting,
            Synchronising,
            Synchronised,
            Ready
        };

        bool loaded = false;
        bool gotLocalStrings = false;
        bool gotRemoteStrings = false;
        State state = State::NotConnected;
        std::optional<String> curMessage;

        using Result = LocalisationClient::StringsResult;
        std::optional<Result> localStrings;
        std::optional<Result> remoteStrings;
        Future<Result> localStringsFuture;
        Future<Result> remoteStringsFuture;

        void load();
        void loadOriginalDataFromDisk();

        void tryLoading();
        void populateData();
        void populateOriginalLanguageData();
        void populateTranslationData();
        void addTranslationData(UIWidget& container, const LocOriginalData& origData, const LocTranslationData& translationData, int totalKeys, int totalWords, bool canEdit);

        LocOriginalData& getOriginalData();
        const LocOriginalData& getOriginalData() const;
        LocTranslationData* getTranslationData(const I18NLanguage& language);

        void openOriginalLanguage(bool canEdit);
        void openLanguage(const I18NLanguage& language, bool canEdit);
        void exportLanguage(const I18NLanguage& language);
        void importLanguage(const I18NLanguage& language);

        bool isDevEnvironment() const;
        bool canViewLanguage(const I18NLanguage& language) const;
        bool canEditLanguage(const I18NLanguage& language) const;

        void signIn(const String& username, const String& password);
        void signOut();
        void onConnected(LocalisationClient::LoginResult result);

		void uploadOriginalStrings();
        void downloadTranslations();
    };
}
