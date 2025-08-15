#pragma once

#include "localisation_remote_client_updater.h"

namespace Halley {
	class ProjectWindow;
	class LocalisationEditor;

	class LocalisationEditorRoot : public UIWidget {
    public:
        LocalisationEditorRoot(ProjectWindow& projectWindow, UIFactory& factory, const HalleyAPI& api);

        void drillDown(std::shared_ptr<UIWidget> widget);
        void returnToRoot();

        void update(Time t, bool moved) override;

        LocalisedString getLanguageName(const I18NLanguage& language, bool includeCountry = true) const;
        Sprite getFlag(const I18NLanguage& language) const;

    private:
        Project& project;
        UIFactory& factory;

        std::shared_ptr<LocalisationEditor> editor;
        std::shared_ptr<UIWidget> curWidget;

        HashMap<String, String> countryNames;
        HashMap<String, String> languageNames;
        HashSet<String> languageNeedsQualifier;

		LocalisationRemoteClientUpdater remoteClientUpdater;

        void setupCountryNames();
        void setupLanguageNames();
    };
}
