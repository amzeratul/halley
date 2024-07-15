#pragma once

#include "halley/ui/ui_widget.h"

namespace Halley {
    class LocalisationEditor : public UIWidget {
    public:
        LocalisationEditor(Project& project, UIFactory& factory);

        void update(Time t, bool moved) override;
        void onMakeUI() override;
        void onActiveChanged(bool active) override;

    private:
        Project& project;
        UIFactory& factory;
        I18NLanguage originalLanguage;

        bool loaded = false;
        int totalCount = 0;
        HashMap<String, int> wordCounts;

        HashMap<String, String> countryNames;
        HashMap<String, String> languageNames;
        HashSet<String> languageNeedsQualifier;

        void load();

        void wordCount();
        static int getWordCount(const String& line);
        String getCategory(const String& assetId) const;
        String getLanguageName(const I18NLanguage& language) const;
        Sprite getFlag(const I18NLanguage& language) const;
        String getNumberWithCommas(int number) const;

        void populateData();
        void addTranslationData(UIWidget& container, const I18NLanguage& language);

        void setupCountryNames();
        void setupLanguageNames();
    };
}
