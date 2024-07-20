#pragma once

namespace Halley {
	class LocalisationEditor;

	class LocalisationEditorRoot : public UIWidget {
    public:
        LocalisationEditorRoot(Project& project, UIFactory& factory);

        void drillDown(std::shared_ptr<UIWidget> widget);
        void returnToRoot();

        LocalisedString getLanguageName(const I18NLanguage& language) const;
        Sprite getFlag(const I18NLanguage& language) const;

    private:
        Project& project;
        UIFactory& factory;

        std::shared_ptr<LocalisationEditor> editor;
        std::shared_ptr<UIWidget> curWidget;

        HashMap<String, String> countryNames;
        HashMap<String, String> languageNames;
        HashSet<String> languageNeedsQualifier;

        void setupCountryNames();
        void setupLanguageNames();
    };
}
