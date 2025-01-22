#pragma once

#include "localisation_client.h"
#include "localisation_data.h"
#include "halley/tools/project/project.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
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
        LocalisationEditor(LocalisationEditorRoot& root, Project& project, UIFactory& factory, const HalleyAPI& api);

        void update(Time t, bool moved) override;
        void onMakeUI() override;
        void onActiveChanged(bool active) override;

        void onAssetsLoaded() override;

    private:
        LocalisationEditorRoot& root;
        Project& project;
        UIFactory& factory;
        const HalleyAPI& api;

        std::unique_ptr<LocalisationClient> client;

        LocOriginalData originalLanguage;
        HashMap<String, LocTranslationData> localised;

        struct Result {
        	LocOriginalData originalLanguage;
			HashMap<String, LocTranslationData> localised;
        };

        bool loaded = false;
        Future<Result> waitingToPopulate;

        void load();

        void loadFromResources();

        void requestPopulateData();
        void populateData();
        void addTranslationData(UIWidget& container, const I18NLanguage& language, int totalKeys, int totalWords, bool canEdit);

        void openLanguage(LocTranslationData* localisationData, bool canEdit);

        bool canViewLanguage(const I18NLanguage& language) const;
        bool canEditLanguage(const I18NLanguage& language) const;

        void uploadOriginalStrings();
    };
}
