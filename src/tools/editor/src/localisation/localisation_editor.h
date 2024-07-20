#pragma once

#include "localisation_data.h"
#include "halley/tools/project/project.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
	class LocalisationEditorRoot;

	class LocalisationEditor : public UIWidget, public Project::IAssetLoadListener, public ILocalisationInfoRetriever {
    public:
        LocalisationEditor(LocalisationEditorRoot& root, Project& project, UIFactory& factory);

        void update(Time t, bool moved) override;
        void onMakeUI() override;
        void onActiveChanged(bool active) override;

        void onAssetsLoaded() override;

    private:
        LocalisationEditorRoot& root;
        Project& project;
        UIFactory& factory;

        LocalisationData originalLanguage;
        HashMap<String, LocalisationData> localised;

        bool loaded = false;

        void load();

        void loadFromResources();
        String getCategory(const String& assetId) const override;
        String getNumberWithCommas(int number) const;

        void populateData();
        void addTranslationData(UIWidget& container, const I18NLanguage& language, int totalKeys, bool canEdit);

        void openLanguage(LocalisationData& localisationData, bool canEdit);
    };
}
